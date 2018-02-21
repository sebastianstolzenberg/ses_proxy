//
// Created by ses on 18.02.18.
//

#include <functional>
#include <iostream>
#include <boost/lexical_cast.hpp>

#include "net/client/connection.hpp"
#include "net/jsonrpc/jsonrpc.hpp"
#include "proxy/pool.hpp"

namespace ses {
namespace proxy {

void Pool::connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass,
             net::ConnectionType connectionType)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  connection_ = net::client::establishConnection(shared_from_this(), host, port, connectionType);

  sendRequest(REQUEST_TYPE_LOGIN, stratum::client::createLoginRequest(user, pass, "ses-proxy"));
}

void Pool::getJob()
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  sendRequest(REQUEST_TYPE_GETJOB);
}

void Pool::submit(const std::string& nonce, const std::string& result)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
  if (currentJob_)
  {
    sendRequest(REQUEST_TYPE_SUBMIT,
                stratum::client::createSubmitRequest(clientIdentifier_, currentJob_->getJobId(), nonce, result));
  }
}

void Pool::handleReceived(char* data, std::size_t size)
{
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
            stratum::client::parseSubmitResponse(result, error,
                                                 std::bind(&Pool::handleSubmitSuccess, this, _1),
                                                 std::bind(&Pool::handleSubmitError, this, _1, _2));
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

void Pool::handleLoginSuccess(const std::string& id, const stratum::Job::Ptr& job)
{
  std::cout << "proxy::Pool::handleLoginSuccess, id, " << id << std::endl;

  clientIdentifier_ = id;
  if (job)
  {
    currentJob_ = job;
    currentJob_->setNonce(0x88000099);
  }
}

void Pool::handleLoginError(int code, const std::string& message)
{
  std::cout << "proxy::Pool::handleLoginError, code, " << code << ", message, " << message<< std::endl;
}

void Pool::handleGetJobSuccess(const stratum::Job::Ptr& job)
{
  std::cout << "proxy::Pool::handleGetJobSuccess" << std::endl;
}

void Pool::handleGetJobError(int code, const std::string& message)
{
  std::cout << "proxy::Pool::handleGetJobError, code, " << code << ", message, " << message << std::endl;
}

void Pool::handleSubmitSuccess(const std::string& status)
{
  std::cout << "proxy::Pool::handleSubmitSuccess, status, " << status << std::endl;
}

void Pool::handleSubmitError(int code, const std::string& message)
{
  std::cout << "proxy::Pool::handleSubmitError, code, " << code << ", message, " << message << std::endl;
}

void Pool::handleNewJob(const stratum::Job::Ptr& job)
{
  std::cout << "proxy::Pool::handleNewJob, job.jobId_, " << job->getJobId() << std::endl;
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

  RequestIdentifier id = nextRequestIdentifier_;
  ++nextRequestIdentifier_;
  outstandingRequests_[id] = type;
  connection_->send(net::jsonrpc::request(std::to_string(id), method, params));
}

} // namespace proxy
} // namespace ses