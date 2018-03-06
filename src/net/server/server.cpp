//
// Created by ses on 16.02.18.
//

#include <iostream>
#include <thread>
#include <boost/asio.hpp>

#include "net/server/server.hpp"

namespace ses {
namespace net {
namespace server {

class BoostConnection : public Connection
{
public:
  explicit BoostConnection(boost::asio::ip::tcp::socket socket)
    : socket_(std::move(socket))
  {
    triggerRead();
  }

public:
  virtual bool connected() const
  {
    return socket_.lowest_layer().is_open();
  }

  virtual std::string connectedIp() const
  {
    return connected() ?
           socket_.lowest_layer().remote_endpoint().address().to_string() :
           "";
  }

  virtual bool send(const char* data, std::size_t size)
  {
    std::cout << "net::server::BoostConnection::send:  ";
    std::cout.write(data, size);
    std::cout << "\n";

    socket_.send(boost::asio::buffer(data, size));
  }

private:
  void triggerRead()
  {
    boost::asio::async_read(socket_,
                            boost::asio::buffer(receiveBuffer_, sizeof(receiveBuffer_)),
                            boost::asio::transfer_at_least(1),
                            [this](boost::system::error_code error, size_t bytes_transferred)
                            {
                              if (!error)
                              {
                                std::cout << "net::server::BoostConnection received :";
                                std::cout.write(receiveBuffer_, bytes_transferred);
                                std::cout << "\n";
                                notifyRead(receiveBuffer_, bytes_transferred);
                                triggerRead();
                              }
                              else
                              {
                                std::cout << "net::server::BoostConnection Read failed: " << error.message() << "\n";
                                notifyError(error.message());
                              }
                            });
  }

private:
  boost::asio::ip::tcp::socket socket_;
  char receiveBuffer_[2048];
};

class BoostServer : public Server
{
public:
  BoostServer(const std::shared_ptr<boost::asio::io_service>& ioService,
              const NewConnectionHandler& handler, const std::string& address, uint16_t port)
    : newConnecionHandler_(handler)
    , ioService_(ioService)
    , acceptor_(*ioService_)
    , nextSocket_(*ioService_)
  {
    //TODO signal handling

    boost::asio::ip::tcp::resolver resolver(*ioService_);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({address, std::to_string(port)});
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    accept();
  }

private:
  void accept()
  {
    acceptor_.async_accept(
      nextSocket_,
      [this](boost::system::error_code ec)
      {
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open())
        {
          return;
        }

        if (!ec && newConnecionHandler_)
        {
          newConnecionHandler_(std::make_shared<BoostConnection>(std::move(nextSocket_)));
        }

      accept();
      });
  }

private:
  std::shared_ptr<boost::asio::io_service> ioService_;
  NewConnectionHandler newConnecionHandler_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket nextSocket_;
};

Server::Ptr createServer(const std::shared_ptr<boost::asio::io_service>& ioService,
                         const NewConnectionHandler& handler, const EndPoint& endPoint)
{
  return std::make_shared<BoostServer>(ioService, handler, endPoint.host_, endPoint.port_);
}

} //namespace server
} //namespace net
} //namespace ses