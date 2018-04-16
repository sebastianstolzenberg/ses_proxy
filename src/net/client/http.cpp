#include <memory>
#include <string>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>

#include "net/client/boosttlsconnection.hpp"
#include "net/endpoint.hpp"

namespace ses {
namespace net {
namespace client {

namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

class Http : public std::enable_shared_from_this<Http>
{
public:
  enum ResponseError
  {

  };

  typedef std::function<void(const std::string& data)> ResponseHandler;
  typedef std::function<void(const std::string& error)> ErrorHandler;

public:
  Http(const std::shared_ptr<boost::asio::io_service>& ioService,
       const EndPoint& endPoint, const std::string& userAgent)
    : ioService_(ioService), endPoint_(endPoint), socket_(*ioService), userAgent_(userAgent)
  {

  }

  void connect()
  {
    connection_ =
        establishBoostTlsConnection(ioService_, endPoint_.host_, endPoint_.port_,
                                    std::bind(&Http::handleConnect, this),
                                    std::bind(&Http::handleReceived, this, std::placeholders::_1),
                                    std::bind(&Http::handleDisconnect, this, std::placeholders::_1));
  }

  void post(const std::string& url, const std::string& body,
            const ResponseHandler responseHandler, const ErrorHandler errorHandler)
  {
    boost::beast::http::request<boost::beast::http::string_body> request
        { boost::beast::http::verb::post, url, 11 };

    request.set(boost::beast::http::field::host, endPoint_.host_);
    request.set(boost::beast::http::field::user_agent, userAgent_);

    request.body() = body;

    auto connection = connection_;
    boost::beast::http::async_write(
        connection->getSocket(), request,
        [connection, responseHandler, errorHandler]
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
                    responseHandler(response->body());
                  }
                });
          }
        });
  }


private:
  void handleConnect()
  {

  }

  void handleReceived(const std::string& data)
  {

  }

  void handleDisconnect(const std::string& error)
  {

  }



  std::shared_ptr<boost::asio::io_service> ioService_;
  net::EndPoint endPoint_;
  std::string userAgent_;

  boost::asio::ip::tcp::socket socket_;
  BoostTlsConnection::Ptr connection_;
};

} //namespace client
} //namespace net
} //namespace ses