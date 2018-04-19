#include <memory>
#include <string>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>

#include "net/client/boosttlsconnection.hpp"
#include "net/client/boosttcpconnection.hpp"
#include "net/client/http.hpp"

namespace ses {
namespace net {
namespace client {

namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

Http::Http(const std::shared_ptr<boost::asio::io_service>& ioService,
     const EndPoint& endPoint, const std::string& userAgent)
  : ioService_(ioService), endPoint_(endPoint), userAgent_(userAgent)
{

}

void Http::setBearerAuthenticationToken(const std::string& token)
{
  if (token.empty())
  {
    bearerAuthenticationToken_.clear();
  }
  else
  {
    bearerAuthenticationToken_ = std::string("Bearer ") + token;
  }
}

void Http::connect(ConnectHandler connectHandler, ErrorHandler errorHandler)
{
//  connection_ =
//      establishBoostTlsConnection(ioService_, endPoint_.host_, endPoint_.port_,
//                                  connectHandler,
//                                  [](const std::string&) {/*ignored*/},
//                                  errorHandler);
  connection_ =
    establishBoostTcpConnection(ioService_, endPoint_.host_, endPoint_.port_,
                                connectHandler,
                                [](const std::string&) {/*ignored*/},
                                errorHandler);

}

void Http::post(const std::string& url, const std::string& contentType, const std::string& body,
          const ResponseHandler responseHandler, const ErrorHandler errorHandler)
{
  auto request = std::make_shared<http::request<http::string_body> >(http::verb::post, url, 11);

  request->set(http::field::host, endPoint_.host_);
  request->set(http::field::user_agent, userAgent_);
  request->set(http::field::content_type, contentType);

  if (!bearerAuthenticationToken_.empty())
  {
    request->set(http::field::authorization, bearerAuthenticationToken_);
  }

  request->body() = body;
  request->prepare_payload();

  LOG_TRACE << "Http::post() request: " << *request;

//  auto connection = std::dynamic_pointer_cast<BoostTlsConnection>(connection_);
  auto connection = std::dynamic_pointer_cast<BoostTcpConnection>(connection_);
  http::async_write(
      connection->getSocket(), *request,
      [connection, responseHandler, errorHandler, request]
          (boost::system::error_code error, std::size_t bytes_transferred)
      {
        if (error)
        {
          errorHandler(error.message());
        }
        else
        {
          auto buffer = std::make_shared<boost::beast::flat_buffer>();
          auto response = std::make_shared<http::response<http::string_body> >();
          http::async_read(
              connection->getSocket(), *buffer, *response,
              [responseHandler, errorHandler, buffer, response]
                  (boost::system::error_code error, std::size_t bytes_transferred)
              {
                if (error)
                {
                  errorHandler(error.message());
                }
                else
                {
                  LOG_TRACE << "Http::post() response: " << *response;
                  auto statusClass = http::to_status_class(response->result());
                  if (statusClass != http::status_class::successful)
                  {
                    std::stringstream header;
                    header << response->base();
                    errorHandler(header.str());
                  }
                  else
                  {
                    responseHandler(response->body());
                  }
                }
              });
        }
      });
}

void Http::handleConnect()
{

}

void Http::handleReceived(const std::string& data)
{

}

void Http::handleDisconnect(const std::string& error)
{

}

} //namespace client
} //namespace net
} //namespace ses