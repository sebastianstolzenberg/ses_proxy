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

#define LOG_POOL_INFO LOG_INFO << poolName_ << ": "

namespace ses {
namespace proxy {

//TODO solo mode with reservedOffset

Pool::Pool(const std::shared_ptr<boost::asio::io_service>& ioService)
  : ioService_(ioService), keepaliveTimer_(*ioService_)
{
}

void Pool::connect(const Configuration& configuration)
{
  LOG_INFO << "Connecting new pool - " << configuration;
  configuration_ = configuration;
  connect();
  updateName();
}

Pool::~Pool()
{
  if (connection_)
  {
    connection_->disconnect();
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
      LOG_POOL_INFO << "Added worker " << worker->getIdentifier() << ", hashRate, " << worker->getHashRate()
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
    LOG_POOL_INFO << "Removed worker " << worker->getIdentifier() << ", hashRate, " << worker->getHashRate()
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

Algorithm Pool::getAlgotrithm() const
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

void Pool::handleConnect()
{
  LOG_POOL_INFO << "Connected";
  login();
  connection_->startReading();
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
      LOG_DEBUG << "proxy::Pool::handleReceived request, id, " << id << ", method, " << method
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
  LOG_POOL_INFO << "Disconnected: " << error;
  connection_.reset();

  // tries to reconnect in 5 seconds
  std::weak_ptr<Pool> weakSelf = shared_from_this();
  auto timer = std::make_shared<boost::asio::deadline_timer>(*ioService_);
  timer->expires_from_now(boost::posix_time::seconds(5));
  timer->async_wait(
    [this, weakSelf, timer](const boost::system::error_code& error)
    {
      if (!error && !weakSelf.expired())
      {
        connect();
      }
    });
}

void Pool::handleLoginSuccess(const std::string& id, const boost::optional<stratum::Job>& job)
{
  LOG_TRACE << "proxy::Pool::handleLoginSuccess, id, " << id;

  workerIdentifier_ = id;
  updateName();
  if (job)
  {
    setJob(*job);
  }
}

void Pool::handleLoginError(int code, const std::string& message)
{
  LOG_ERROR << poolName_ << ": " << "proxy::Pool::handleLoginError, code, " << code << ", message, " << message<< std::endl;
}

void Pool::handleGetJobSuccess(const stratum::Job& job)
{
  LOG_TRACE << "proxy::Pool::handleGetJobSuccess";
  setJob(job);
}

void Pool::handleGetJobError(int code, const std::string& message)
{
  LOG_ERROR << poolName_ << ": " << "proxy::Pool::handleGetJobError, code, " << code << ", message, " << message;
}

void Pool::handleSubmitSuccess(const std::string& jobId, const JobResult::SubmitStatusHandler& submitStatusHandler,
                               const std::string& status)
{
  submitHashRate_.addHashes(activeJobTemplate_->getDifficulty());

  LOG_POOL_INFO << "Submit success: job, " << jobId << ", " << submitHashRate_;


  if (submitStatusHandler)
  {
    submitStatusHandler(JobResult::SUBMIT_ACCEPTED);
  }

  auto job = jobTemplates_[jobId];
  if (job)
  {
    LOG_DEBUG << "proxy::Pool, job " << job->getJobIdentifier() << ", numHashes " << job->numHashesFound();
  }
}

void Pool::handleSubmitError(const std::string& jobId, const JobResult::SubmitStatusHandler& submitStatusHandler,
                             int code, const std::string& message)
{
  LOG_ERROR << poolName_ << ": " << "Submit failed: message, " << code << " " << message << ", job, " << jobId;

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
  LOG_TRACE << "proxy::Pool::handleNewJob, job.jobIdentifier_, " << job.getJobIdentifier();
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
  poolNameStream << "<" << workerIdentifier_ << "@"
                 << configuration_.endPoint_.host_ << ":" << configuration_.endPoint_.port_ << ">";
  poolName_ = poolNameStream.str();
}

void Pool::connect()
{
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
      auto newJobTemplate = JobTemplate::create(workerIdentifier_, job);
      LOG_POOL_INFO << "Received new job: " << *newJobTemplate;
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
    LOG_DEBUG << __PRETTY_FUNCTION__ << " Known job template, setting it as active job template";
    activateJob(knownJob->second);
  }
  else
  {
    LOG_DEBUG << __PRETTY_FUNCTION__ << " Known job template which is active already";
  }
}

void Pool::activateJob(const JobTemplate::Ptr& job)
{
  LOG_POOL_INFO << "Activating job: " << *job;
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
  if (worker && activeJobTemplate_)
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
      LOG_POOL_INFO << "Submitting hash: job, " << jobIt->second->getJobIdentifier()
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
      LOG_POOL_INFO << "Submitting hash: job, " << jobIt->second->getJobIdentifier()
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
  keepaliveTimer_.expires_from_now(boost::posix_time::seconds(30));
  keepaliveTimer_.async_wait(
      [this](const boost::system::error_code& error)
      {
        if (!error)
        {
          sendRequest(REQUEST_TYPE_KEEPALIVE,
                      stratum::client::createKeepalivedParams(workerIdentifier_));
          triggerKeepaliveTimer();
        }
      });
}

} // namespace proxy
} // namespace ses