#pragma once

#include <memory>

#include "net/endpoint.hpp"
#include "net/connection.hpp"

namespace ses {
namespace net {
namespace client {


class Http : public std::enable_shared_from_this<Http>
{
public:
  typedef std::shared_ptr<Http> Ptr;
  typedef std::function<void()> ConnectHandler;
  typedef std::function<void(const std::string& data)> ResponseHandler;
  typedef std::function<void(const std::string& error)> ErrorHandler;

public:
  Http(const std::shared_ptr<boost::asio::io_service>& ioService,
       const EndPoint& endPoint, const std::string& userAgent);

  ~Http();

  void setBearerAuthenticationToken(const std::string& token);

  void connect(ConnectHandler connectHandler, ErrorHandler errorHandler);

  void disconnect();

  void post(const std::string& url, const std::string& contentType, const std::string& body,
            ResponseHandler responseHandler, ErrorHandler errorHandler);


private:
  void handleConnect();
  void handleReceived(const std::string& data);
  void handleDisconnect(const std::string& error);

  std::shared_ptr<boost::asio::io_service> ioService_;
  net::EndPoint endPoint_;
  std::string userAgent_;
  std::string bearerAuthenticationToken_;


  Connection::Ptr connection_;
};

} //namespace client
} //namespace net
} //
