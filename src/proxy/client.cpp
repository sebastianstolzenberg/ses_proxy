
#include <iostream>

#include "net/jsonrpc/jsonrpc.hpp"
#include "stratum/stratum.hpp"
#include "proxy/client.hpp"

namespace ses {
namespace proxy {

Client::Client(const std::shared_ptr<boost::asio::io_service>& ioService,
               const WorkerIdentifier& id, Algorithm defaultAlgorithm)
  : ioService_(ioService), identifier_(id), algorithm_(defaultAlgorithm), type_(WorkerType::UNKNOWN)
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

WorkerType Client::getType() const
{
  return type_;
}

void Client::assignJob(const Job::Ptr& job)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  if (job)
  {
//    job->setAssignedWorker(identifier_);
    jobs_[job->getJobIdentifier()] = job;
    currentJob_ = job;
    sendJobNotification();
  }
}

bool Client::canHandleJobTemplates() const
{
  return type_ == WorkerType::PROXY;
}

void Client::assignJobTemplate(const JobTemplate::Ptr& job)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  if (job)
  {
    //job->setAssignedWorker(identifier_);
    jobTemplates_[job->getJobIdentifier()] = job;
    currentJobTemplate_ = job;
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

  std::cout << __PRETTY_FUNCTION__ << std::endl;

  net::jsonrpc::parse(
    std::string(data, size),
    [this](const std::string& id, const std::string& method, const std::string& params)
    {
//      std::cout << "proxy::Client::handleReceived request, id, " << id << ", method, " << method
//                << ", params, " << params << std::endl;
      stratum::server::parseRequest(id, method, params,
                                    std::bind(&Client::handleLogin, this, _1, _2, _3, _4),
                                    std::bind(&Client::handleGetJob, this, _1),
                                    std::bind(&Client::handleSubmit, this, _1, _2, _3, _4, _5, _6, _7),
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
  std::cout << __PRETTY_FUNCTION__ << " " << error << std::endl;
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

    if (useragent_.find("xmr-node-proxy") != std::string::npos)
    {
      type_ = WorkerType::PROXY;
    }
    else
    {
      type_ = WorkerType::MINER;
    }
    //TODO agent can contain "NiceHash", which asks for a certain difficulty (check nodejs code)

    //TODO login parsing: <wallet>[.<payment id>][+<difficulty>]
    //TODO password parsing: <identifier>[:<email>]

    std::string responseResult =
      stratum::server::createLoginResponse(boost::uuids::to_string(identifier_),
                                           currentJob_ ? currentJob_->asStratumJob() : std::optional<stratum::Job>());

    std::string response = net::jsonrpc::response(jsonRequestId, responseResult, "");
    std::cout << " response = " << response << std::endl;

    connection_->send(response);
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
                          const std::string& nonce, const std::string& result,
                          const std::string& workerNonce, const std::string& poolNonce)
{
  //TODO extend for JobTemplate receiver
  std::cout << __PRETTY_FUNCTION__ << std::endl
            << " identifier = " << identifier << std::endl
            << " jobIdentifier = " << jobIdentifier << std::endl
            << " nonce = " << nonce << std::endl
            << " result = " << result << std::endl
            << " workerNonce = " << workerNonce << std::endl
            << " poolNonce = " << poolNonce << std::endl;

  auto jobIt = jobs_.find(jobIdentifier);
  if (jobIt != jobs_.end())
  {
    jobIt->second->submitResult(JobResult(jobIdentifier, nonce, result),
                                std::bind(&Client::handleUpstreamSubmitStatus, shared_from_this(),
                                          jsonRequestId, std::placeholders::_1));
  }
  else
  {
    sendErrorResponse(jsonRequestId, "Invalid job id");
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

void Client::handleUpstreamSubmitStatus(std::string jsonRequestId, JobResult::SubmitStatus submitStatus)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  switch (submitStatus)
  {
    case JobResult::SUBMIT_REJECTED_IP_BANNED:
      sendErrorResponse(jsonRequestId, "IP Address currently banned");
      break;

    case JobResult::SUBMIT_REJECTED_UNAUTHENTICATED:
      sendErrorResponse(jsonRequestId, "Unauthenticated");
      break;

    case JobResult::SUBMIT_REJECTED_DUPLICATE:
      sendErrorResponse(jsonRequestId, "Duplicate share");
      break;

    case JobResult::SUBMIT_REJECTED_EXPIRED:
      sendErrorResponse(jsonRequestId, "Block expired");
      break;

    case JobResult::SUBMIT_REJECTED_INVALID_JOB_ID:
      sendErrorResponse(jsonRequestId, "Invalid job id");
      break;

    case JobResult::SUBMIT_REJECTED_LOW_DIFFICULTY_SHARE:
      sendErrorResponse(jsonRequestId, "Low difficulty share");
      break;

    case JobResult::SUBMIT_ACCEPTED:
    default:
      sendSuccessResponse(jsonRequestId, "OK");
      break;
  }
}

void Client::sendSuccessResponse(const std::string& jsonRequestId, const std::string& status)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  connection_->send(net::jsonrpc::statusResponse(jsonRequestId, status));
}

void Client::sendErrorResponse(const std::string& jsonRequestId, const std::string& message)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  connection_->send(net::jsonrpc::errorResponse(jsonRequestId, -1, message));
}

void Client::sendJobNotification()
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  if (currentJob_)
  {
    connection_->send(
      net::jsonrpc::notification("job", stratum::server::createJobNotification(currentJob_->asStratumJob())));

  }
}

} // namespace proxy
} // namespace ses