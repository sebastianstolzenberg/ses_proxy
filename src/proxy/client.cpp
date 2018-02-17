
#include <iostream>
#include "proxy/client.hpp"

namespace ses {
namespace proxy {

Client::Client()
{
}

void Client::setConnection(const net::Connection::Ptr& connection)
{
  connection_ = connection;
  connection_->setHandler(shared_from_this());
}

void Client::handleReceived(char* data, std::size_t size)
{
  std::cout << "proxy::Client::handleReceived" << std::string(data, size) << std::endl;

  net::jsonrpc::parse(*this, std::string(data, size));
}

void Client::handleError(const std::string& error)
{

}

void Client::handleJsonRequest(const std::string& id, const std::string& method, const std::string& params)
{
  std::cout << "proxy::Client::handleJsonRequest, id, " << id << ", method, " << method << ", params, " << params << std::endl;

  stratum::parseServerMethod(id, *this, method, params);
}

void Client::handleJsonResponse(const std::string& id, const std::string& result, const std::string& error)
{
  std::cout << "proxy::Client::handleJsonResponse, id, " << id << ", result, " << result << ", error, " << error << std::endl;
}

void Client::handleJsonNotification(const std::string& id, const std::string& method, const std::string& params)
{
  std::cout << "proxy::Client::handleJsonNotification, id, " << id << ", method, " << method << ", params, " << params << std::endl;

  stratum::parseServerMethod(id, *this, method, params);
}


void Client::handleStratumServerLogin(const std::string& jsonRequestId,
                                      const std::string& login, const std::string& pass, const std::string& agent)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl
            << " login = " << login << std::endl
            << " pass = " << pass << std::endl
            << " agent = " << agent << std::endl;

  stratum::Job job = {"0100fce3a2d4053f5d3b6d35992eca02a91c0e9789633dbc97e82074d26ee7816323fb313c795f00000000ec114de588d63bce2dcfd18f04ea3664942350ad63a752387212adc57ade160805",
                      "9Zl7oF0WaLk1IRz577c2vhY5ebs5",
                      "26310800",
                      "b78f6a5e-2e3f-4b46-b13c-223ca7b23fcd"};

  std::string responseResult =
    stratum::createLoginResponse("b78f6a5e-2e3f-4b46-b13c-223ca7b23fcd", job, "OK");

  std::string response = net::jsonrpc::response(jsonRequestId, responseResult, "");
  std::cout << " response = " << response << std::endl;

  connection_->send(response);

  connection_->send(net::jsonrpc::notification("job", stratum::createJobNotification(job)));
}

void Client::handleStratumServerGetJob(const std::string& jsonRequestId)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Client::handleStratumServerSubmit(const std::string& jsonRequestId,
                                       const std::string& identifier, const std::string& jobIdentifier,
                                       const std::string& nonce, const std::string& result)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl
            << " identifier = " << identifier << std::endl
            << " jobIdentifier = " << jobIdentifier << std::endl
            << " nonce = " << nonce << std::endl
            << " result = " << result << std::endl;
  connection_->send(net::jsonrpc::okResponse(jsonRequestId));
}

void Client::handleStratumServerKeepAliveD(const std::string& jsonRequestId)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}


void Client::handleStratumServerMiningAuthorize(const std::string& username, const std::string& password)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Client::handleStratumServerMiningCapabilities(const std::string& tbd)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Client::handleStratumMServeriningExtraNonceSubscribe()
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Client::handleStratumServerMiningGetTransactions(const std::string& jobIdentifier)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Client::handleStratumServerMiningSubmit(const std::string& username, const std::string& jobIdentifier,
                                             const std::string& extraNonce2, const std::string& nTime,
                                             const std::string& nOnce)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Client::handleStratumServerMiningSubscribe(const std::string& userAgent, const std::string& extraNonce1)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Client::handleStratumServerMiningSuggestDifficulty(const std::string& difficulty)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void Client::handleStratumServerMiningSuggestTarget(const std::string& target)
{
  std::cout << __PRETTY_FUNCTION__ << std::endl;
}


} // namespace proxy
} // namespace ses