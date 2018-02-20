
#include <iostream>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include "net/jsonrpc/jsonrpc.hpp"
#include "stratum/stratum.hpp"
#include "proxy/client.hpp"

namespace ses {
namespace proxy {

Client::Client(const boost::uuids::uuid& id)
  : rpcIdentifier_(id)
{
}

void Client::setConnection(const net::Connection::Ptr& connection)
{
  connection_ = connection;
  connection_->setHandler(shared_from_this());
}

void Client::handleReceived(char* data, std::size_t size)
{
  using namespace std::placeholders;

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
  }
  else
  {
    // TODO 'invalid address used for login'

    stratum::Job
      job =
      {"0100fce3a2d4053f5d3b6d35992eca02a91c0e9789633dbc97e82074d26ee7816323fb313c795f00000000ec114de588d63bce2dcfd18f04ea3664942350ad63a752387212adc57ade160805",
       "9Zl7oF0WaLk1IRz577c2vhY5ebs5",
       "26310800",
       boost::uuids::to_string(rpcIdentifier_)};

    std::string responseResult =
      stratum::server::createLoginResponse(boost::uuids::to_string(rpcIdentifier_));

    std::string response = net::jsonrpc::response(jsonRequestId, responseResult, "");
    std::cout << " response = " << response << std::endl;

    connection_->send(response);

    connection_->send(net::jsonrpc::notification("job", stratum::server::createJobNotification(job)));
  }
}

void Client::handleGetJob(const std::string& jsonRequestId)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
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

  if (!identifier.empty() && rpcIdentifier_ != boost::lexical_cast<boost::uuids::uuid>(identifier))
  {
    sendErrorResponse(jsonRequestId, "Unauthenticated");
  }
  else
  {
//    if (invalid job id)
//    {
//      sendErrorResponse(jsonRequestId, "Invalid job id");
//    }
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
//    else
    {
      //TODO test nonce pattern -> sendErrorResponse(jsonRequestId, "Duplicate share");
      //TODO block expired -> sendErrorResponse(jsonRequestId, "Block expired");
      //TODO low difficulty share -> sendErrorResponse(jsonRequestId, "Low difficulty share");

      sendSuccessResponse(jsonRequestId, "OK");
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

void Client::sendSuccessResponse(const std::string& jsonRequestId, const std::string& status)
{
  connection_->send(net::jsonrpc::statusResponse(jsonRequestId, status));
}

void Client::sendErrorResponse(const std::string& jsonRequestId, const std::string& message)
{
  connection_->send(net::jsonrpc::errorResponse(jsonRequestId, -1, message));
}

} // namespace proxy
} // namespace ses