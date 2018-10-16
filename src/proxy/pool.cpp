#include <iostream>
#include <functional>
#include <boost/lexical_cast.hpp>
#include <boost/range/numeric.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "net/client/connection.hpp"
#include "net/jsonrpc/jsonrpc.hpp"
#include "proxy/pool.hpp"
#include "util/log.hpp"

#undef LOG_COMPONENT
#define LOG_COMPONENT pool

#define LOG_POOL_ERROR LOG_ERROR << COLOR_PURPLE << poolShortName_ << " -> " << COLOR_NC
#define LOG_POOL_INFO LOG_INFO << COLOR_PURPLE << poolShortName_ << " -> " << COLOR_NC
#define LOG_POOL_DEBUG LOG_DEBUG << COLOR_PURPLE << poolShortName_ << " -> " << COLOR_NC
#define LOG_POOL_TRACE LOG_TRACE << COLOR_PURPLE << poolShortName_ << " -> " << COLOR_NC

namespace ses {
namespace proxy {

//TODO solo mode with reservedOffset

Pool::Pool(const std::shared_ptr<boost::asio::io_service>& ioService)
  : ioService_(ioService), keepaliveTimer_(*ioService_),
    totalSubmits_(0), successfulSubmits_(0)
{
}

Pool::~Pool()
{
  disconnect();
}

void Pool::connect(const Configuration& configuration)
{
  LOG_INFO << "Connecting new pool - " << configuration;
  configuration_ = configuration;
  connect();
  updateName();
}

void Pool::disconnect()
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  LOG_DEBUG << "Disconnecting from pool and releasing all " << workers_.size() << " workers";

  if (connection_)
  {
    connection_->resetHandler();
    connection_->disconnect();
    connection_.reset();
  }

  jobTemplates_.clear();

  std::list<Worker::Ptr> workers(std::move(workers_));
  for (auto& worker : workers)
  {
    // informs the worker about the invalidated job
    worker->revokeJob();
  }
}

bool Pool::addWorker(const Worker::Ptr& worker)
{
  bool accepted = false;
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto workerIt = std::find(workers_.begin(), workers_.end(), worker);
  if (workerIt != workers_.end())
  {
    // already assigned to this pool
    accepted = true;
  }
  else
  {
    accepted = assignJobToWorker(worker);
    if (accepted)
    {
      workers_.push_back(worker);
      LOG_POOL_DEBUG << "Added worker " << worker->getIdentifier() << ", hashRate, " << worker->getHashRate()
                     << ", numWorkers, " << workers_.size();

    }
  }
  return accepted;
}

bool Pool::removeWorker(const Worker::Ptr& worker)
{
  bool removed = false;
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto workerIt = std::find(workers_.begin(), workers_.end(), worker);
  if (workerIt != workers_.end())
  {
    workers_.erase(workerIt);
    removed = true;
    LOG_POOL_DEBUG << "Removed worker " << worker->getIdentifier() << ", hashRate, " << worker->getHashRate()
                   << ", numWorkers, " << workers_.size();
  }
  return removed;
}

void Pool::removeAllWorkers()
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  workers_.clear();
}

const std::list<Worker::Ptr>& Pool::getWorkersSortedByHashrateDescending()
{
  workers_.sort([](const auto& a, const auto& b)
                {
                  return a->getHashRate().getAverageHashRateLongTimeWindow() >
                         b->getHashRate().getAverageHashRateLongTimeWindow();
                });
  return workers_;
}

const std::string& Pool::getDescriptor() const
{
  return poolName_;
}

const std::string& Pool::getName() const
{
  return configuration_.name_;
}

Algorithm Pool::getAlgorithm() const
{
  return configuration_.algorithm_;
}

double Pool::getWeight() const
{
  return configuration_.weight_;
}

size_t Pool::numWorkers() const
{
  return workers_.size();
}

double Pool::weightedWorkers() const
{
  double weightedWorkers = 0;
  if (getWeight() > 0)
  {
    weightedWorkers = numWorkers();
    weightedWorkers /= getWeight();
  }
  return weightedWorkers;
}

uint32_t Pool::hashRate() const
{
  return boost::accumulate(workers_, 0,
                           [](uint32_t sum, const auto& worker)
                           {
                             return sum + worker->getHashRate().getAverageHashRateLongTimeWindow();
                           });
}

double Pool::weightedHashRate() const
{
  double weightedHashRate = 0;
  if (getWeight() > 0)
  {
    weightedHashRate = hashRate();
    weightedHashRate /= getWeight();
  }
  return weightedHashRate;
}

const util::HashRateCalculator& Pool::getWorkerHashRate()
{
  workerHashRate_.addHashRate(hashRate());
  return workerHashRate_;
}

const util::HashRateCalculator& Pool::getSubmitHashRate()
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  // ensures hashrates drop to zero when nothing gets submitted for a while
  submitHashRate_.addHashes(0);
  return submitHashRate_;
}

CcClient::Status Pool::getCcStatus()
{
  ccStatus_.hashRateShort_ = hashRate();
  ccStatus_.hashRateMedium_ = getWorkerHashRate().getAverageHashRateShortTimeWindow();
  ccStatus_.hashRateLong_ = getWorkerHashRate().getAverageHashRateLongTimeWindow();
  ccStatus_.hashRateHighest_ = std::max(ccStatus_.hashRateHighest_, ccStatus_.hashRateShort_);
  ccStatus_.sharesTotal_ = totalSubmits_;
  ccStatus_.sharesGood_ = successfulSubmits_;
  ccStatus_.hashesTotal_ = getWorkerHashRate().getTotalHashes();
  ccStatus_.numMiners_ = numWorkers();
  std::ostringstream clientId;
  ccStatus_.clientId_ = clientId.str();
  ccStatus_.currentPool_ = poolShortName_;
  ccStatus_.currentAlgoName_ = toString(configuration_.algorithm_.getAlgorithmType_());
  ccStatus_.upTime_ = getWorkerHashRate().secondsSinceStart().count();
  ccStatus_.upTime_ *= 1000;
  return ccStatus_;
}

void Pool::handleConnect()
{
  LOG_POOL_INFO << "Connected";
  if (connection_)
  {
    login();
    connection_->startReading();
  }
  else
  {
    // something is wrong, got a callback without having a valid connection ... reconnect
    connect();
  }
}

void Pool::handleReceived(const std::string& data)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

//  LOG_DEBUG << __PRETTY_FUNCTION__;
  using namespace std::placeholders;

  net::jsonrpc::parse(
    data,
    [this](const std::string& id, const std::string& method, const std::string& params)
    {
      LOG_POOL_DEBUG << "proxy::Pool::handleReceived request, id, " << id << ", method, " << method
                     << ", params, " << params;
    },
    [this](const std::string& id, const std::string& result, const std::string& error)
    {
      RequestIdentifier requestId = boost::lexical_cast<RequestIdentifier>(id);
      if (outstandingRequests_.count(requestId) > 0)
      {
        switch (outstandingRequests_[requestId])
        {
          case REQUEST_TYPE_LOGIN:
            stratum::client::parseLoginResponse(result, error,
                                                std::bind(&Pool::handleLoginSuccess, this, _1, _2),
                                                std::bind(&Pool::handleLoginError, this, _1, _2));
            break;

          case REQUEST_TYPE_GETJOB:
            stratum::client::parseGetJobResponse(result, error,
                                                std::bind(&Pool::handleGetJobSuccess, this, _1),
                                                std::bind(&Pool::handleGetJobError, this, _1, _2));
            break;

          case REQUEST_TYPE_SUBMIT:
          {
            std::string jobId;
            JobResult::SubmitStatusHandler submitStatusHandler;
            std::tie(jobId, submitStatusHandler) = outstandingSubmits_[requestId];
            outstandingSubmits_.erase(requestId);
            stratum::client::parseSubmitResponse(result, error,
                                                 std::bind(&Pool::handleSubmitSuccess, this, jobId, submitStatusHandler, _1),
                                                 std::bind(&Pool::handleSubmitError, this, jobId, submitStatusHandler, _1, _2));
            break;
          }

          default:
            break;
        }
        outstandingRequests_.erase(requestId);
      }
    },
    [this](const std::string& method, const std::string& params)
    {
      stratum::client::parseNotification(method, params, std::bind(&Pool::handleNewJob, this, _1));
    });
  triggerKeepaliveTimer();
}

void Pool::handleDisconnect(const std::string& error)
{
  LOG_POOL_ERROR << "Disconnected: " << error;

  disconnect();

  // tries to reconnect in 5 seconds
  std::weak_ptr<Pool> weakSelf = shared_from_this();
  auto timer = std::make_shared<boost::asio::deadline_timer>(*ioService_);
  timer->expires_from_now(boost::posix_time::seconds(5));
  timer->async_wait(
    [weakSelf, timer, this](const boost::system::error_code& error)
    {
      auto lockedSelf = weakSelf.lock();
      if (!error && lockedSelf)
      {
        lockedSelf->connect();
      }
    });
}

void Pool::handleLoginSuccess(const std::string& id, const boost::optional<stratum::Job>& job)
{
  LOG_POOL_TRACE << "proxy::Pool::handleLoginSuccess, id, " << id;

  workerIdentifier_ = id;
  updateName();
  if (job)
  {
    setJob(*job);
  }
}

void Pool::handleLoginError(int code, const std::string& message)
{
  LOG_POOL_ERROR << "proxy::Pool::handleLoginError, code, " << code << ", message, " << message;
}

void Pool::handleGetJobSuccess(const stratum::Job& job)
{
  LOG_POOL_TRACE << "proxy::Pool::handleGetJobSuccess";
  setJob(job);
}

void Pool::handleGetJobError(int code, const std::string& message)
{
  LOG_POOL_ERROR << "proxy::Pool::handleGetJobError, code, " << code << ", message, " << message;
}

void Pool::handleSubmitSuccess(const std::string& jobId, const JobResult::SubmitStatusHandler& submitStatusHandler,
                               const std::string& status)
{
  submitHashRate_.addHashes(activeJobTemplate_->getDifficulty());

  ++totalSubmits_;
  ++successfulSubmits_;

  if (submitStatusHandler)
  {
    submitStatusHandler(JobResult::SUBMIT_ACCEPTED);
  }

  auto job = jobTemplates_[jobId];
  if (job)
  {
    LOG_POOL_INFO << COLOR_GREEN << "Accepted" << COLOR_NC << " ["  << successfulSubmits_ << "/" << totalSubmits_-successfulSubmits_ << "], "
                  << "Diff: " << job->getDifficulty() << ", "
                  << submitHashRate_;
  }
  else
  {
    LOG_POOL_INFO << COLOR_GREEN << "Accepted" << COLOR_NC << " ["  << successfulSubmits_ << "/" << totalSubmits_-successfulSubmits_ << "], "
                  << submitHashRate_;
  }
}

void Pool::handleSubmitError(const std::string& jobId, const JobResult::SubmitStatusHandler& submitStatusHandler,
                             int code, const std::string& message)
{
  ++totalSubmits_;

  auto job = jobTemplates_[jobId];
  if (job)
  {
    LOG_POOL_ERROR << "Rejected [" << successfulSubmits_ << "/" << totalSubmits_-successfulSubmits_ << "], "
                   << "Diff: " << job->getDifficulty() << ", "
		   << "Reason: " << message;
  }
  else
  {
    LOG_POOL_ERROR << "Rejected [" << successfulSubmits_ << "/" << totalSubmits_-successfulSubmits_ << "], "
                   << "Reason: " << message;
  }

  if (submitStatusHandler)
  {
    JobResult::SubmitStatus status = JobResult::SUBMIT_REJECTED_INVALID_JOB_ID;
    if (message == "Unauthenticated")
    {
      status = JobResult::SUBMIT_REJECTED_UNAUTHENTICATED;
      login();
    }
    else if (message == "IP Address currently banned")
    {
      status = JobResult::SUBMIT_REJECTED_IP_BANNED;
    }
    else if (message == "Duplicate share")
    {
      status = JobResult::SUBMIT_REJECTED_DUPLICATE;
    }
    else if (message == "Block expired")
    {
      status = JobResult::SUBMIT_REJECTED_EXPIRED;
      removeJob(jobId);
      //TODO send new job to miner
    }
    else if (message == "Invalid job id")
    {
      status = JobResult::SUBMIT_REJECTED_INVALID_JOB_ID;
      removeJob(jobId);
    }
    else if (message == "Low difficulty share")
    {
      status = JobResult::SUBMIT_REJECTED_LOW_DIFFICULTY_SHARE;
    }
    submitStatusHandler(status);
  }
}

void Pool::handleNewJob(const stratum::Job& job)
{
  LOG_POOL_TRACE << "proxy::Pool::handleNewJob, job.jobIdentifier_, " << job.getJobIdentifier();
  setJob(job);
}

JobResult::SubmitStatus Pool::handleJobResult(const std::string& workerIdentifier,
                                              const JobResult& jobResult,
                                              const JobResult::SubmitStatusHandler& submitStatusHandler)
{
  LOG_DEBUG << __PRETTY_FUNCTION__;

  submit(jobResult, submitStatusHandler);
}

void Pool::updateName()
{
  std::ostringstream poolNameStream;
  if (!configuration_.name_.empty())
  {
    poolNameStream << configuration_.name_;
  }
  else
  {
    poolNameStream << configuration_.endPoint_.host_ << ":" << configuration_.endPoint_.port_;
  }
  poolShortName_ = poolNameStream.str();

  poolNameStream = std::ostringstream();
  if (!configuration_.name_.empty())
  {
    poolNameStream << configuration_.name_ << " ";
  }
  poolNameStream << "<" << workerIdentifier_ << "@"
                 << configuration_.endPoint_.host_ << ":" << configuration_.endPoint_.port_ << ">";
  poolName_ = poolNameStream.str();
}

void Pool::connect()
{
  disconnect();
  connection_ = net::client::establishConnection(ioService_,
                                                 configuration_.endPoint_,
                                                 std::bind(&Pool::handleConnect, this),
                                                 std::bind(&Pool::handleReceived, this,
                                                           std::placeholders::_1),
                                                 std::bind(&Pool::handleDisconnect, this,
                                                           std::placeholders::_1));
}

Pool::RequestIdentifier Pool::sendRequest(Pool::RequestType type, const std::string& params)
{
  if (connection_)
  {
    std::string method;
    switch (type)
    {
      case REQUEST_TYPE_LOGIN:
        method = "login";
        break;

      case REQUEST_TYPE_GETJOB:
        method = "getjob";
        break;

      case REQUEST_TYPE_SUBMIT:
        method = "submit";
        break;

      case REQUEST_TYPE_KEEPALIVE:
        method = "keepalived";
        break;

      default:
        // unknown request type
        return 0;
    }

    auto id = nextRequestIdentifier_;
    ++nextRequestIdentifier_;
    outstandingRequests_[id] = type;
    connection_->send(net::jsonrpc::request(std::to_string(id), method, params));
    triggerKeepaliveTimer();
    return id;
  }
  else
  {
    return 0;
  }
}

void Pool::setJob(const stratum::Job& job)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  auto knownJob = jobTemplates_.find(job.getJobIdentifier());
  if (knownJob == jobTemplates_.end())
  {
    try
    {
      auto newJobTemplate = JobTemplate::create(workerIdentifier_, getAlgorithm(), job);
      LOG_POOL_INFO << COLOR_NC << "Received new job with target diff: " << COLOR_BLUE << job.getTargetDiff() << COLOR_NC;
      newJobTemplate->setJobResultHandler(std::bind(&Pool::handleJobResult, shared_from_this(),
                                                    std::placeholders::_1, std::placeholders::_2,
                                                    std::placeholders::_3));
      jobTemplates_[newJobTemplate->getJobIdentifier()] = newJobTemplate;
      activateJob(newJobTemplate);
    }
    catch (...)
    {
      LOG_CURRENT_EXCEPTION;
    }
  }
  else if (activeJobTemplate_ && job.getJobIdentifier() != activeJobTemplate_->getJobIdentifier())
  {
    LOG_POOL_DEBUG << " Known job template, setting it as active job template";
    activateJob(knownJob->second);
  }
  else
  {
    LOG_POOL_DEBUG << " Known job template which is active already";
  }
}

void Pool::activateJob(const JobTemplate::Ptr& job)
{
  LOG_POOL_DEBUG << "Activating job: " << *job;
  activeJobTemplate_ = job;
  for (const auto& worker : workers_)
  {
    assignJobToWorker(worker);
  }
}

void Pool::removeJob(const std::string& jobId)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  // invalidates job and removes it from lists
  auto job = jobTemplates_.find(jobId);
  if (job != jobTemplates_.end())
  {
    //job->second->invalidate();
    jobTemplates_.erase(job);
  }
  if (activeJobTemplate_ && activeJobTemplate_->getJobIdentifier() == jobId)
  {
    activeJobTemplate_.reset();
  }
}

bool Pool::assignJobToWorker(const Worker::Ptr& worker)
{
  bool accepted = false;
  if (worker && activeJobTemplate_ &&
      worker->supports(getAlgorithm()))
  {
    //TODO PoW aware worker selection
    auto job = activeJobTemplate_->getJobFor(worker->getIdentifier(), worker->getType());
    if (job)
    {
      worker->assignJob(job);
      accepted = true;
    }
  }
  return accepted;
}

void Pool::login()
{
  sendRequest(REQUEST_TYPE_LOGIN,
              stratum::client::createLoginRequest(configuration_.user_,
                                                  configuration_.pass_,
                                                  "ses-proxy/0.1 with xmr-node-proxy support"));
}

void Pool::submit(const JobResult& jobResult, const JobResult::SubmitStatusHandler& submitStatusHandler)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  LOG_DEBUG << __PRETTY_FUNCTION__;

  auto jobIt = jobTemplates_.find(jobResult.getJobIdentifier());
  if (jobIt != jobTemplates_.end())
  {
    //TODO further result verification
    //TODO job template specific response
    RequestIdentifier id = 0;
    uint32_t resultDifficulty = jobResult.getDifficulty();
 
    if (jobResult.isNodeJsResult())
    {
      LOG_POOL_DEBUG << "Submitting hash: job, " << jobIt->second->getJobIdentifier()
                     << ", difficulty, " << resultDifficulty
                     << ", nonce, " << jobResult.getNonceHexString()
                     << ", workerNonce, " << jobResult.getWorkerNonce()
                     << ", poolNonce, " << jobResult.getPoolNonce()
                     << ", hash, " << jobResult.getHashHexString();
      id = sendRequest(REQUEST_TYPE_SUBMIT,
                       stratum::client::createSubmitParams(workerIdentifier_,
                                                           jobIt->second->getJobIdentifier(),
                                                           jobResult.getNonceHexString(),
                                                           jobResult.getHashHexString(),
                                                           jobResult.getWorkerNonce(),
                                                           jobResult.getPoolNonce()));
    }
    else
    {
      LOG_POOL_DEBUG << "Submitting hash: job, " << jobIt->second->getJobIdentifier()
                     << ", difficulty, " << resultDifficulty
                     << ", nonce, " << jobResult.getNonceHexString()
                     << ", hash, " << jobResult.getHashHexString();
      id = sendRequest(REQUEST_TYPE_SUBMIT,
                       stratum::client::createSubmitParams(workerIdentifier_,
                                                           jobIt->second->getJobIdentifier(),
                                                           jobResult.getNonceHexString(),
                                                           jobResult.getHashHexString()));
    }
    if (id != 0)
    {
      outstandingSubmits_[id] = std::tie(jobResult.getJobIdentifier(), submitStatusHandler);
    }
  }
  else
  {
    submitStatusHandler(JobResult::SUBMIT_REJECTED_INVALID_JOB_ID);
    //TODO send new job to miner
  }
}

void Pool::triggerKeepaliveTimer()
{
  auto self = shared_from_this();
  keepaliveTimer_.expires_from_now(boost::posix_time::seconds(30));
  keepaliveTimer_.async_wait(
      [self](const boost::system::error_code& error)
      {
        if (!error)
        {
          std::lock_guard<std::recursive_mutex> lock(self->mutex_);
          if (!self->workerIdentifier_.empty())
          {
            self->sendRequest(REQUEST_TYPE_KEEPALIVE,
                              stratum::client::createKeepalivedParams(self->workerIdentifier_));
          }
          self->triggerKeepaliveTimer();
        }
      });
}

} // namespace proxy
} // namespace ses
