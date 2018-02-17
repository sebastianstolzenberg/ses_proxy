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
                                std::cout << "server::BoostConnection received :";
                                std::cout.write(receiveBuffer_, bytes_transferred);
                                std::cout << "\n";
                                notifyRead(receiveBuffer_, bytes_transferred);
                                triggerRead();
                              }
                              else
                              {
                                std::cout << "Read failed: " << error.message() << "\n";
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
  BoostServer(const ServerHandler::Ptr& handler, const std::string& address, uint16_t port)
    : handler_(handler)
    , acceptor_(ioService_)
    , nextSocket_(ioService_)
  {
    //TODO signal handling

    boost::asio::ip::tcp::resolver resolver(ioService_);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({address, std::to_string(port)});
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    accept();

    std::thread([this]() { ioService_.run(); }).detach();
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

        if (!ec)
        {
          ServerHandler::Ptr handler = handler_.lock();
          if (handler)
          {
            handler->handleNewConnection(
              std::make_shared<BoostConnection>(std::move(nextSocket_)));
          }
          else
          {
            // noone there to handle a new socket ... just closes it
            nextSocket_.close();
          }
        }

      accept();
      });
  }

private:
  ServerHandler::WeakPtr handler_;

  boost::asio::io_service ioService_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket nextSocket_;
};

Server::Ptr createServer(const ServerHandler::Ptr& handler,
                         const std::string& address, uint16_t port,
                         ConnectionType type)
{
  return std::make_shared<BoostServer>(handler, address, port);
}

} //namespace server
} //namespace net
} //namespace ses