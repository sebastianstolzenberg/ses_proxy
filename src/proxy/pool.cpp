//
// Created by ses on 18.02.18.
//

#include <iostream>
#include <functional>
#include <boost/lexical_cast.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "net/client/connection.hpp"
#include "net/jsonrpc/jsonrpc.hpp"
#include "proxy/pool.hpp"

namespace ses {
namespace proxy {

//TODO solo mode with reservedOffset

void Pool::connect(const Configuration& configuration)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  connection_ = net::client::establishConnection(configuration.endPoint_,
                                                 std::bind(&Pool::handleReceived, this,
                                                           std::placeholders::_1, std::placeholders::_2),
                                                 std::bind(&Pool::handleError, this, std::placeholders::_1));
  configuration_ = configuration;
  login();
}

bool Pool::addWorker(const Worker::Ptr& worker)
{
  bool accepted = assignJobToWorker(worker);
  if (accepted)
  {
    worker_.push_back(worker);
  }
  return accepted;
}

void Pool::handleReceived(char* data, std::size_t size)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  std::cout << __PRETTY_FUNCTION__ << std::endl;
  using namespace std::placeholders;

  net::jsonrpc::parse(
    std::string(data, size),
    [this](const std::string& id, const std::string& method, const std::string& params)
    {
      std::cout << "proxy::Pool::handleReceived request, id, " << id << ", method, " << method
                << ", params, " << params << std::endl;
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
}

void Pool::handleError(const std::string& error)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Pool::handleLoginSuccess(const std::string& id, const std::optional<stratum::Job>& job)
{
  std::cout << "proxy::Pool::handleLoginSuccess, id, " << id << std::endl;

  workerIdentifier_ = id;
  if (job)
  {
    setJob(*job);
  }
}

void Pool::handleLoginError(int code, const std::string& message)
{
  std::cout << "proxy::Pool::handleLoginError, code, " << code << ", message, " << message<< std::endl;
}

void Pool::handleGetJobSuccess(const stratum::Job& job)
{
  std::cout << "proxy::Pool::handleGetJobSuccess" << std::endl;
  setJob(job);
}

void Pool::handleGetJobError(int code, const std::string& message)
{
  std::cout << "proxy::Pool::handleGetJobError, code, " << code << ", message, " << message << std::endl;
}

void Pool::handleSubmitSuccess(const std::string& jobId, const JobResult::SubmitStatusHandler& submitStatusHandler,
                               const std::string& status)
{
  std::cout << "proxy::Pool::handleSubmitSuccess, status, " << status << std::endl;
  if (submitStatusHandler)
  {
    submitStatusHandler(JobResult::SUBMIT_ACCEPTED);
  }

  auto job = jobTemplates_[jobId];
  if (job)
  {
    std::cout << "proxy::Pool, job " << job->getJobIdentifier() << ", numHashes " << job->numHashesFound() << std::endl;
  }
}

void Pool::handleSubmitError(const std::string& jobId, const JobResult::SubmitStatusHandler& submitStatusHandler,
                             int code, const std::string& message)
{
  std::cout << "proxy::Pool::handleSubmitError, code, " << code << ", message, " << message << std::endl;

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
  std::cout << "proxy::Pool::handleNewJob, job.jobIdentifier_, " << job.getJobIdentifier() << std::endl;
  setJob(job);
}

JobResult::SubmitStatus Pool::handleJobResult(const WorkerIdentifier& workerIdentifier,
                                              const JobResult& jobResult,
                                              const JobResult::SubmitStatusHandler& submitStatusHandler)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;

  submit(jobResult, submitStatusHandler);
}

Pool::RequestIdentifier Pool::sendRequest(Pool::RequestType type, const std::string& params)
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

    default:
      // unknown request type
      return 0;
  }

  auto id = nextRequestIdentifier_;
  ++nextRequestIdentifier_;
  outstandingRequests_[id] = type;
  connection_->send(net::jsonrpc::request(std::to_string(id), method, params));
  return id;
}

void Pool::setJob(const stratum::Job& job)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  std::cout << __PRETTY_FUNCTION__
            << ", jobId, " << job.getJobIdentifier()
            << ", target, " << job.getTarget()
            << std::endl;

  auto knownJob = jobTemplates_.find(job.getJobIdentifier());
  if (knownJob == jobTemplates_.end())
  {
    std::cout << __PRETTY_FUNCTION__ << " New job template" << std::endl;
    try
    {
      auto newJobTemplate = JobTemplate::create(job);
      newJobTemplate->setJobResultHandler(std::bind(&Pool::handleJobResult, shared_from_this(),
                                                    std::placeholders::_1, std::placeholders::_2,
                                                    std::placeholders::_3));
      jobTemplates_[newJobTemplate->getJobIdentifier()] = newJobTemplate;
      activateJob(newJobTemplate);
    }
    catch (...)
    {
      std::cout << boost::current_exception_diagnostic_information();
    }
  }
  else if (activeJobTemplate_ && job.getJobIdentifier() != activeJobTemplate_->getJobIdentifier())
  {
    std::cout << __PRETTY_FUNCTION__ << " Known job template, setting it as active job template" << std::endl;
    activateJob(knownJob->second);
  }
  else
  {
    std::cout << __PRETTY_FUNCTION__ << " Known job template which is active already" << std::endl;
  }
}

void Pool::activateJob(const JobTemplate::Ptr& job)
{
  activeJobTemplate_ = job;
  for (const auto& worker : worker_)
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
  std::cout << __PRETTY_FUNCTION__ << std::endl;

  auto jobIt = jobTemplates_.find(jobResult.getJobIdentifier());
  if (jobIt != jobTemplates_.end())
  {
    //TODO further result verification
    //TODO job template specific response
    RequestIdentifier id =
      sendRequest(REQUEST_TYPE_SUBMIT,
                stratum::client::createSubmitRequest(workerIdentifier_, jobIt->second->getJobIdentifier(),
                                                     jobResult.getNonceHexString(), jobResult.getHashHexString(),
                                                     jobResult.getWorkerNonceHexString(), jobResult.getPoolNonceHexString()));
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

} // namespace proxy
} // namespace ses