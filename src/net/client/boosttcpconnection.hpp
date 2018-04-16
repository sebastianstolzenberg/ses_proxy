#pragma once

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "net/client/boostconnection.hpp"

namespace ses {
namespace net {
namespace client {

class BoostTcpSocket
{
public:
  typedef boost::asio::ip::tcp::socket SocketType;
public:
  BoostTcpSocket(boost::asio::io_service &ioService)
      : socket_(ioService)
  {
  }

  template<class ITERATOR, class HANDLER>
  void connect(ITERATOR &iterator, HANDLER handler)
  {
    boost::asio::async_connect(
        socket_, iterator,
        [handler](const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator)
        {
          handler(error);
        });
  }

  SocketType &get()
  {
    return socket_;
  }

  const SocketType &get() const
  {
    return socket_;
  }

private:
  SocketType socket_;
};

typedef BoostConnection<BoostTcpSocket> BoostTcpConnection;

BoostTcpConnection::Ptr establishBoostTcpConnection(
    const std::shared_ptr<boost::asio::io_service>& ioService,
    const std::string& host, uint16_t port,
    const Connection::ConnectHandler& connectHandler,
    const Connection::ReceivedDataHandler& receivedDataHandler,
    const Connection::DisconnectHandler& errorHandler);

} //namespace client
} //namespace ses
} //namespace net
