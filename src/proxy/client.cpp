
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "net/jsonrpc/jsonrpc.hpp"
#include "stratum/stratum.hpp"
#include "proxy/client.hpp"

namespace ses {
namespace proxy {

Client::Client(const WorkerIdentifier& id, Algorithm defaultAlgorithm)
  : identifier_(id), algorithm_(defaultAlgorithm)
{
}

void Client::setConnection(const net::Connection::Ptr& connection)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  if (connection_)
  {
    connection_->resetHandler();
  }
  connection_ = connection;
  if (connection_)
  {
    connection_->setHandler(std::bind(&Client::handleReceived, this, std::placeholders::_1, std::placeholders::_2),
                            std::bind(&Client::handleError, this, std::placeholders::_1));
  }
}

WorkerIdentifier Client::getIdentifier() const
{
  return identifier_;
}

Algorithm Client::getAlgorithm() const
{
  return algorithm_;
}

void Client::assignJob(const Job::Ptr& job, const Job::JobResultHandler& jobResultHandler)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (job)
  {
    currentJob_ = job;
    jobResultHandler_ = jobResultHandler;
    currentJob_->setAssignedWorker(identifier_);
    sendJobNotification();
  }
}

const std::string& Client::getUseragent() const
{
  return useragent_;
}

const std::string& Client::getUsername() const
{
  return username_;
}

const std::string& Client::getPassword() const
{
  return password_;
}

void Client::handleReceived(char* data, std::size_t size)
{
  using namespace std::placeholders;

  std::lock_guard<std::recursive_mutex> lock(mutex_);

  net::jsonrpc::parse(
    std::string(data, size),
    [this](const std::string& id, const std::string& method, const std::string& params)
    {
//      std::cout << "proxy::Client::handleReceived request, id, " << id << ", method, " << method
//                << ", params, " << params << std::endl;
      stratum::server::parseRequest(id, method, params,
                                    std::bind(&Client::handleLogin, this, _1, _2, _3, _4),
                                    std::bind(&Client::handleGetJob, this, _1),
                                    std::bind(&Client::handleSubmit, this, _1, _2, _3, _4, _5),
                                    std::bind(&Client::handleKeepAliveD, this, _1, _2),
                                    std::bind(&Client::handleUnknownMethod, this, _1));
    },
    [this](const std::string& id, const std::string& result, const std::string& error)
    {
      std::cout << "proxy::Client::handleReceived response, id, " << id << ", result, " << result
                << ", error, " << error << std::endl;
    },
    [this](const std::string& method, const std::string& params)
    {
      std::cout << "proxy::Client::handleReceived notification, method, " << method << ", params, "
                << params << std::endl;
    });
}

void Client::handleError(const std::string& error)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Client::handleLogin(const std::string& jsonRequestId, const std::string& login, const std::string& pass, const std::string& agent)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl
            << " login = " << login << std::endl
            << " pass = " << pass << std::endl
            << " agent = " << agent << std::endl;

  if (login.empty())
  {
    sendErrorResponse(jsonRequestId, "missing login");
    useragent_.clear();
    username_.clear();
    password_.clear();
  }
  else
  {
    // TODO 'invalid address used for login'

    useragent_ = agent;
    username_ = login;
    password_ = pass;

    std::string responseResult =
      stratum::server::createLoginResponse(boost::uuids::to_string(identifier_),
                                           currentJob_ ? currentJob_->asStratumJob() : std::optional<stratum::Job>());

    std::string response = net::jsonrpc::response(jsonRequestId, responseResult, "");
    std::cout << " response = " << response << std::endl;

    connection_->send(response);

    if (currentJob_)
    {
      connection_->send(net::jsonrpc::notification("job",
                                                   stratum::server::createJobNotification(
                                                     currentJob_->asStratumJob())));
    }
  }
}

void Client::handleGetJob(const std::string& jsonRequestId)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;

  if (currentJob_)
  {
    connection_->send(
      net::jsonrpc::response(jsonRequestId,
                             stratum::server::createJobNotification(currentJob_->asStratumJob()),
                             ""));
  }
  else
  {
    sendErrorResponse(jsonRequestId, "No job available");
  }
}

void Client::handleSubmit(const std::string& jsonRequestId,
                          const std::string& identifier, const std::string& jobIdentifier,
                          const std::string& nonce, const std::string& result)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl
            << " identifier = " << identifier << std::endl
            << " jobIdentifier = " << jobIdentifier << std::endl
            << " nonce = " << nonce << std::endl
            << " result = " << result << std::endl;

  if (jobResultHandler_)
  {
    jobResultHandler_(identifier_,
                      JobResult(jobIdentifier, nonce, result),
                      std::bind(&Client::handleUpstreamSubmitStatus, shared_from_this(),
                                jsonRequestId, std::placeholders::_1));
  }

  if (!identifier.empty() && identifier_ != boost::lexical_cast<WorkerIdentifier>(identifier))
  {
    sendErrorResponse(jsonRequestId, "Unauthenticated");
  }
  else
  {
    if (!currentJob_ || currentJob_->getJobId() != jobIdentifier)
    {
      sendErrorResponse(jsonRequestId, "Invalid job id");
    }
//    else
//    if (difficulty too low)
//    {
//      sendErrorResponse(jsonRequestId, "Low difficulty share");
//    }
//    else
//    if (invalid nicehash nonce)
//    {
//      sendErrorResponse(jsonRequestId, "Invalid nonce; is miner not compatible with NiceHash?");
//    }
    else
    {
      //TODO test nonce pattern -> sendErrorResponse(jsonRequestId, "Duplicate share");
      //TODO block expired -> sendErrorResponse(jsonRequestId, "Block expired");
      //TODO low difficulty share -> sendErrorResponse(jsonRequestId, "Low difficulty share");

      if (currentJob_->submitResult(JobResult(jobIdentifier, nonce, result)))
      {
        sendSuccessResponse(jsonRequestId, "OK");
      }
      else
      {
        sendErrorResponse(jsonRequestId, "Rejected");
      }
    }
  }
}

void Client::handleKeepAliveD(const std::string& jsonRequestId, const std::string& identifier)
{
  std::cout << __PRETTY_FUNCTION__ << ", identifier, " << identifier << std::endl;
  sendSuccessResponse(jsonRequestId, "KEEPALIVED");
}

void Client::handleUnknownMethod(const std::string& jsonRequestId)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  sendErrorResponse(jsonRequestId, "invalid method");
}

void Client::handleUpstreamSubmitStatus(std::string jsonRequestId, Job::SubmitStatus submitStatus)
{
  switch (submitStatus)
  {
    case Job::SUBMIT_REJECTED_IP_BANNED:
      sendErrorResponse(jsonRequestId, "IP Address currently banned");
      break;

    case Job::SUBMIT_REJECTED_UNAUTHENTICATED:
      sendErrorResponse(jsonRequestId, "Unauthenticated");
      break;

    case Job::SUBMIT_REJECTED_DUPLICATE:
      sendErrorResponse(jsonRequestId, "Duplicate share");
      break;

    case Job::SUBMIT_REJECTED_EXPIRED:
      sendErrorResponse(jsonRequestId, "Block expired");
      break;

    case Job::SUBMIT_REJECTED_INVALID_JOB_ID:
      sendErrorResponse(jsonRequestId, "Invalid job id");
      break;

    case Job::SUBMIT_REJECTED_LOW_DIFFICULTY_SHARE:
      sendErrorResponse(jsonRequestId, "Low difficulty share");
      break;

    case Job::SUBMIT_ACCEPTED:
    default:
      sendSuccessResponse(jsonRequestId, "OK");
      break;
  }
}

void Client::sendSuccessResponse(const std::string& jsonRequestId, const std::string& status)
{
  connection_->send(net::jsonrpc::statusResponse(jsonRequestId, status));
}

void Client::sendErrorResponse(const std::string& jsonRequestId, const std::string& message)
{
  connection_->send(net::jsonrpc::errorResponse(jsonRequestId, -1, message));
}

void Client::sendJobNotification()
{
  if (currentJob_)
  {
    connection_->send(
      net::jsonrpc::notification("job", stratum::server::createJobNotification(currentJob_->asStratumJob())));

  }
}

} // namespace proxy
} // namespace ses