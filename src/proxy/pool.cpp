//
// Created by ses on 18.02.18.
//

#include <functional>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "net/client/connection.hpp"
#include "net/jsonrpc/jsonrpc.hpp"
#include "proxy/pool.hpp"

namespace ses {
namespace proxy {

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

Job::Ptr Pool::getNextJob()
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;

  Job::Ptr job;
  if (activeJob_)
  {
    job = activeJob_->getNextSubJob();
  }
  else
  {
    sendRequest(REQUEST_TYPE_GETJOB);
  }
  return job;
}

Job::Ptr Pool::getJob(const WorkerIdentifier& workerIdentifier) override
{
  Job::Ptr job;
  if (activeJob_)
  {
    job = activeJob_->getSubJob(workerIdentifier);
  }
  return job;
}

Job::SubmitStatus Pool::submitJobResult(const WorkerIdentifier& workerIdentifier,
                                        const JobResult& jobResult) override
{
  return submit(jobResult);
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
            std::string jobId = outstandingSubmits_[requestId];
            stratum::client::parseSubmitResponse(result, error,
                                                 std::bind(&Pool::handleSubmitSuccess, this, jobId, _1),
                                                 std::bind(&Pool::handleSubmitError, this, jobId, _1, _2));
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

void Pool::handleSubmitSuccess(const::std::string& jobId, const std::string& status)
{
  std::cout << "proxy::Pool::handleSubmitSuccess, status, " << status << std::endl;
}

void Pool::handleSubmitError(const::std::string& jobId, int code, const std::string& message)
{
  std::cout << "proxy::Pool::handleSubmitError, code, " << code << ", message, " << message << std::endl;

  Job::SubmitStatus status = Job::SUBMIT_REJECTED_INVALID_JOB_ID;
  if (message == "Unauthenticated")
  {
    status = Job::SUBMIT_REJECTED_UNAUTHENTICATED;
    login();
  }
  else if (message == "IP Address currently banned")
  {
    status = Job::SUBMIT_REJECTED_IP_BANNED;
  }
  else if (message == "Duplicate share")
  {
    status = Job::SUBMIT_REJECTED_DUPLICATE;
  }
  else if (message == "Block expired")
  {
    status = Job::SUBMIT_REJECTED_EXPIRED;
    removeJob(jobId);
  }
  else if (message == "Invalid job id")
  {
    status = Job::SUBMIT_REJECTED_INVALID_JOB_ID;
    removeJob(jobId);
  }
  else if (message == "Low difficulty share")
  {
    status = Job::SUBMIT_REJECTED_LOW_DIFFICULTY_SHARE;
  }
}

void Pool::handleNewJob(const stratum::Job& job)
{
  std::cout << "proxy::Pool::handleNewJob, job.jobId_, " << job.getJobId() << std::endl;
  setJob(job);
}

Job::SubmitStatus Pool::handleJobResult(const JobResult& jobResult)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;

  submit(jobResult);
}

void Pool::sendRequest(Pool::RequestType type, const std::string& params)
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
      return;
  }

  auto id = nextRequestIdentifier_;
  ++nextRequestIdentifier_;
  outstandingRequests_[id] = type;
  connection_->send(net::jsonrpc::request(std::to_string(id), method, params));
}

void Pool::setJob(const stratum::Job& job)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  std::cout << __PRETTY_FUNCTION__
            << ", jobId, " << job.getJobId()
            << ", target, " << job.getTarget()
            << std::endl;
  if (jobs_.count(job.getJobId()) == 0)
  {
    try
    {
      auto newJob = std::make_shared<MasterJob>(job);
      newJob->setJobResultHandler(std::bind(&Pool::handleJobResult, shared_from_this(), std::placeholders::_1));
      jobs_[newJob->getJobId()] = newJob;
      activeJob_ = newJob;
    }
    catch (...)
    {
      std::cout << boost::current_exception_diagnostic_information();
    }
  }
  else
  {
    // job is known already
  }
}

void Pool::removeJob(const std::string& jobId)
{
  // invalidates job and removes it from lists
  if (jobs_.count(jobId) > 0)
  {
    jobs_[jobId]->invalidate();
    jobs_.erase(jobId);
  }
  if (activeJob_ && activeJob_->getJobId() == jobId)
  {
    activeJob_.reset();
  }
}

void Pool::login()
{
  sendRequest(REQUEST_TYPE_LOGIN,
              stratum::client::createLoginRequest(configuration_.user_, configuration_.pass_, "ses-proxy"));
}

Job::SubmitStatus Pool::submit(const JobResult& jobResult)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  std::cout << __PRETTY_FUNCTION__ << std::endl;

  Job::SubmitStatus submitStatus = Job::SUBMIT_REJECTED_INVALID_JOB_ID;

  auto jobIt = jobs_.find(jobResult.getJobId());
  if (jobIt != jobs_.end())
  {
    //TODO further result verification
    sendRequest(REQUEST_TYPE_SUBMIT,
                stratum::client::createSubmitRequest(workerIdentifier_, jobIt->second->getJobId(),
                                                     jobResult.getNonceHexString(),
                                                     jobResult.getHashHexString()));

    submitStatus = Job::SUBMIT_ACCEPTED;
  }
  else
  {
    submitStatus = Job::SUBMIT_REJECTED_INVALID_JOB_ID;
  }
  return submitStatus;
}

} // namespace proxy
} // namespace ses