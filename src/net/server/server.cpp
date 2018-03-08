//
// Created by ses on 16.02.18.
//

#include <iostream>
#include <thread>
#include <boost/asio.hpp>

#include "net/server/server.hpp"
#include "util/log.hpp"

namespace ses {
namespace net {
namespace server {

class BoostConnection : public Connection
{
public:
  explicit BoostConnection(boost::asio::ip::tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

public:
  virtual bool isConnected() const override
  {
    return socket_.lowest_layer().is_open();
  }

  virtual std::string getConnectedIp() const override
  {
    return isConnected() ? socket_.lowest_layer().remote_endpoint().address().to_string() : "";
  }

  virtual uint16_t getConnectedPort() const override
  {
    return isConnected() ? socket_.lowest_layer().remote_endpoint().port() : 0;
  }

  virtual bool send(const char* data, std::size_t size) override
  {
    LOG_TRACE << "net::server::BoostConnection::send: ";
    LOG_TRACE.write(data, size);

    socket_.send(boost::asio::buffer(data, size));
  }

protected:
  void startReading() override
  {
    triggerRead();
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
                                LOG_TRACE << "net::server::BoostConnection received :";
                                LOG_TRACE.write(receiveBuffer_, bytes_transferred);
                                notifyRead(receiveBuffer_, bytes_transferred);
                                triggerRead();
                              }
                              else
                              {
                                LOG_ERROR << "net::server::BoostConnection Read failed: " << error.message();
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