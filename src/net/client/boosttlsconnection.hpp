#pragma once

#include <boost/asio/connect.hpp>
#include <boost/asio/ssl.hpp>

#include "net/client/boostconnection.hpp"

namespace ses {
namespace net {
namespace client {

class BoostTlsSocket
{
public:
  typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SocketType;

public:
  BoostTlsSocket(boost::asio::io_service &ioService)
      : sslContext_(boost::asio::ssl::context::sslv23_client)
      , socket_(ioService, sslContext_)
  {
    socket_.set_verify_mode(boost::asio::ssl::verify_none);
  }

  template<class ITERATOR, class HANDLER>
  void connect(ITERATOR &iterator, HANDLER handler)
  {
    boost::asio::async_connect(
        socket_.lowest_layer(), iterator,
        [this, handler](const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator)
        {
          if (error)
          {
            handler(error);
          }
          else
          {
            socket_.async_handshake(boost::asio::ssl::stream_base::client, handler);
          }
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
  boost::asio::ssl::context sslContext_;
  SocketType socket_;
};

typedef BoostConnection<BoostTlsSocket> BoostTlsConnection;

BoostTlsConnection::Ptr establishBoostTlsConnection(
    const std::shared_ptr<boost::asio::io_service>& ioService,
    const std::string& host, uint16_t port,
    const Connection::ConnectHandler& connectHandler,
    const Connection::ReceivedDataHandler& receivedDataHandler,
    const Connection::DisconnectHandler& errorHandler);

} //namespace client
} //namespace ses
} //namespace net
