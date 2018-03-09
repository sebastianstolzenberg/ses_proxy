
#include <iostream>

#include "net/jsonrpc/jsonrpc.hpp"
#include "stratum/stratum.hpp"
#include "proxy/client.hpp"
#include "util/log.hpp"

namespace ses {
namespace proxy {

Client::Client(const std::shared_ptr<boost::asio::io_service>& ioService,
               const WorkerIdentifier& id, Algorithm defaultAlgorithm, uint32_t defaultDifficulty)
  : ioService_(ioService), identifier_(id), algorithm_(defaultAlgorithm),
    type_(WorkerType::UNKNOWN), difficulty_(defaultDifficulty)
{
}

void Client::setDisconnectHandler(const DisconnectHandler& disconnectHandler)
{
  disconnectHandler_ = disconnectHandler;
}

void Client::setConnection(const net::Connection::Ptr& connection)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  auto oldConnection = connection_.lock();
  if (oldConnection && oldConnection != connection)
  {
    oldConnection->disconnect();
  }
  connection_ = connection;
  if (connection)
  {
    connection->setSelfSustainingUntilDisconnect(true);
    connection->setHandler(std::bind(&Client::handleReceived, this, std::placeholders::_1, std::placeholders::_2),
                           std::bind(&Client::handleDisconnect, this, std::placeholders::_1));
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
  LOG_DEBUG << __PRETTY_FUNCTION__;
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
  LOG_DEBUG << __PRETTY_FUNCTION__;
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

  net::jsonrpc::parse(
    std::string(data, size),
    [this](const std::string& id, const std::string& method, const std::string& params)
    {
//      LOG_DEBUG << "proxy::Client::handleReceived request, id, " << id << ", method, " << method
//                << ", params, " << params;
      stratum::server::parseRequest(id, method, params,
                                    std::bind(&Client::handleLogin, this, _1, _2, _3, _4),
                                    std::bind(&Client::handleGetJob, this, _1),
                                    std::bind(&Client::handleSubmit, this, _1, _2, _3, _4, _5, _6, _7),
                                    std::bind(&Client::handleKeepAliveD, this, _1, _2),
                                    std::bind(&Client::handleUnknownMethod, this, _1));
    },
    [this](const std::string& id, const std::string& result, const std::string& error)
    {
      LOG_DEBUG << "proxy::Client::handleReceived response, id, " << id << ", result, " << result
                << ", error, " << error;
    },
    [this](const std::string& method, const std::string& params)
    {
      LOG_DEBUG << "proxy::Client::handleReceived notification, method, " << method << ", params, "
                << params;
    });
}

void Client::handleDisconnect(const std::string& error)
{
  LOG_TRACE << __PRETTY_FUNCTION__ << " " << error;
  disconnectHandler_();
}

void Client::handleLogin(const std::string& jsonRequestId, const std::string& login, const std::string& pass, const std::string& agent)
{
  LOG_DEBUG << "ses::proxy::Client::handleLogin()"
            << ", login, " << login
            << ", pass, " << pass
            << ", agent, " << agent;

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
    sendResponse(jsonRequestId, responseResult);
  }
}

void Client::handleGetJob(const std::string& jsonRequestId)
{
  LOG_DEBUG << __PRETTY_FUNCTION__;

  if (currentJob_)
  {
    sendResponse(jsonRequestId, stratum::server::createJobNotification(currentJob_->asStratumJob()));
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
  LOG_DEBUG << "ses::proxy::Client::handleSubmit()"
            << ", identifier, " << identifier
            << ", jobIdentifier, " << jobIdentifier
            << ", nonce, " << nonce
            << ", result, " << result
            << ", workerNonce, " << workerNonce
            << ", poolNonce, " << poolNonce;

  if (identifier != toString(identifier_))
  {
    sendErrorResponse(jsonRequestId, "Unauthenticated");
  }
  else
  {
    auto jobIt = jobs_.find(jobIdentifier);
    if (jobIt != jobs_.end())
    {
      JobResult jobResult(jobIdentifier, nonce, result);
      uint32_t resultDifficulty = jobResult.getDifficulty();
      LOG_DEBUG << " difficulty = " << jobResult.getDifficulty();

      if (resultDifficulty < difficulty_)
      {
        sendErrorResponse(jsonRequestId, "Low difficulty share");
      }
      else
      {
        jobIt->second->submitResult(jobResult,
                                    std::bind(&Client::handleUpstreamSubmitStatus,
                                              shared_from_this(),
                                              jsonRequestId, std::placeholders::_1));
      }
    }
    else
    {
      sendErrorResponse(jsonRequestId, "Invalid job id");
    }
  }
}

void Client::handleKeepAliveD(const std::string& jsonRequestId, const std::string& identifier)
{
  LOG_DEBUG << "ses::proxy::Client::handleKeepAliveD(), identifier, " << identifier;
  if (identifier != toString(identifier_))
  {
    sendErrorResponse(jsonRequestId, "Unauthenticated");
  }
  else
  {
    sendSuccessResponse(jsonRequestId, "KEEPALIVED");
  }
}

void Client::handleUnknownMethod(const std::string& jsonRequestId)
{
  sendErrorResponse(jsonRequestId, "invalid method");
}

void Client::handleUpstreamSubmitStatus(std::string jsonRequestId, JobResult::SubmitStatus submitStatus)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

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
    {
      sendSuccessResponse(jsonRequestId, "OK");
      std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
      std::chrono::milliseconds diff =
          std::chrono::duration_cast<std::chrono::milliseconds>(now - lastShareTimePoint_);
      lastShareTimePoint_ = now;
      shareTimeDiffs_.push_back(diff);
      LOG_DEBUG << "Client submit success "
                << (connection_.expired() ? "<no-ip>" : connection_.lock()->getConnectedIp())
                << ", td, " << diff.count() << "ms";
      break;
    }
  }
}

void Client::sendResponse(const std::string& jsonRequestId, const std::string& response)
{
  if (auto connection = connection_.lock())
  {
    connection->send(net::jsonrpc::response(jsonRequestId, response, ""));
  }
}

void Client::sendSuccessResponse(const std::string& jsonRequestId, const std::string& status)
{
  if (auto connection = connection_.lock())
  {
    connection->send(net::jsonrpc::statusResponse(jsonRequestId, status));
  }
}

void Client::sendErrorResponse(const std::string& jsonRequestId, const std::string& message)
{
  if (auto connection = connection_.lock())
  {
    connection->send(net::jsonrpc::errorResponse(jsonRequestId, -1, message));
  }
}

void Client::sendJobNotification()
{
  auto connection = connection_.lock();
  if (currentJob_ && connection)
  {
    connection->send(
      net::jsonrpc::notification("job", stratum::server::createJobNotification(currentJob_->asStratumJob())));
    lastShareTimePoint_ = std::chrono::system_clock::now();
  }
}

} // namespace proxy
} // namespace ses