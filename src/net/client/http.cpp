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

namespace {
template <class Connection, class Request>
void sendReveiceAsync(const std::shared_ptr<Connection> connection, Request& request,
                      const Http::ResponseHandler responseHandler,
                      const Http::ErrorHandler errorHandler)
{
  if (connection)
  {
    http::async_write(
        connection->getSocket(), *request,
        [connection, responseHandler, errorHandler, request]
            (boost::system::error_code error, std::size_t bytes_transferred)
        {
          if (error)
          {
            std::ostringstream err;
            err << "Http sendReveiceAsync() Write error: " << error.value() << ": " << error.message();
            errorHandler(err.str());
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
                    std::ostringstream err;
                    err << "Http sendReveiceAsync() Read error: " << error.value() << ": " << error.message();
                    errorHandler(err.str());
                  }
                  else
                  {
                    LOG_TRACE << "Http::post() response: " << *response;
                    auto statusClass = http::to_status_class(response->result());
                    if (statusClass != http::status_class::successful)
                    {
                      std::stringstream header;
                      header << response->base();
                      std::ostringstream err;
                      err << "Http sendReveiceAsync() Http result: " << header.str();
                      errorHandler(err.str());
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
}
}

Http::Http(const std::shared_ptr<boost::asio::io_service>& ioService,
     const EndPoint& endPoint, const std::string& userAgent)
  : ioService_(ioService), endPoint_(endPoint), userAgent_(userAgent)
{
  if (endPoint_.connectionType_ == net::CONNECTION_TYPE_AUTO)
  {
    endPoint_.connectionType_ = endPoint_.port_ == 443 ?
                                net::CONNECTION_TYPE_TLS :
                                net::CONNECTION_TYPE_TCP;
  }
}

Http::~Http()
{
  disconnect();
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
  if (endPoint_.connectionType_ == net::CONNECTION_TYPE_TLS)
  {
    connection_ =
      establishBoostTlsConnection(ioService_, endPoint_.host_, endPoint_.port_,
                                  connectHandler,
                                  [](const std::string&) {/*ignored*/},
                                  errorHandler);
  }
  else
  {
    connection_ =
      establishBoostTcpConnection(ioService_, endPoint_.host_, endPoint_.port_,
                                  connectHandler,
                                  [](const std::string&) {/*ignored*/},
                                  errorHandler);
  }

}

void Http::disconnect()
{
  if (connection_)
  {
    connection_->resetHandler();
    connection_->disconnect();
    connection_.reset();
  }
}

bool Http::isConnected()
{
  return connection_ && connection_->isConnected();
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

  if (endPoint_.connectionType_ == net::CONNECTION_TYPE_TLS)
  {
    auto connection = std::dynamic_pointer_cast<BoostTlsConnection>(connection_);
    sendReveiceAsync(connection, request, responseHandler, errorHandler);
  }
  else
  {
    auto connection = std::dynamic_pointer_cast<BoostTcpConnection>(connection_);
    sendReveiceAsync(connection, request, responseHandler, errorHandler);
  }
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