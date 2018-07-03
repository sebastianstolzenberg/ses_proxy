#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "net/client/connection.hpp"
#include "util/log.hpp"

#undef LOG_COMPONENT
#define LOG_COMPONENT net

namespace ses {
namespace net {
namespace client {

template<class SOCKET>
class BoostConnection : public Connection, public std::enable_shared_from_this<BoostConnection<SOCKET> >
{
public:
  typedef std::shared_ptr<BoostConnection<SOCKET> > Ptr;

public:
  BoostConnection(const std::shared_ptr<boost::asio::io_service>& ioService,
                  const Connection::ConnectHandler& connectHandler,
                  const Connection::ReceivedDataHandler& receivedDataHandler,
                  const Connection::DisconnectHandler& errorHandler)
    : Connection(receivedDataHandler, errorHandler), connectHandler_(connectHandler),
      ioService_(ioService), socket_(std::make_shared<SOCKET>(*ioService))
  {
  }

  ~BoostConnection()
  {
  }

  typename SOCKET::SocketType& getSocket()
  {
    return socket_->get();
  }

  void connect(const std::string &server, uint16_t port)
  {
    try
    {
      LOG_TRACE << "Connecting net::client::BoostConnection to " << server << ":" << port;
      auto resolver = std::make_shared<boost::asio::ip::tcp::resolver>(*ioService_);
      boost::asio::ip::tcp::resolver::query query(server, std::to_string(port));

      auto self = this->shared_from_this();
      resolver->async_resolve(
        query,
        [self, resolver](const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator iterator)
        {
          if (error)
          {
            self->notifyError(error.message());
          }
          else
          {
            self->socket_->connect(
              iterator,
              [self](const boost::system::error_code& error)
              {
                if (error)
                {
                  self->notifyError(error.message());
                }
                else
                {
                  self->socket_->get().lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
                  self->socket_->get().lowest_layer().set_option(boost::asio::socket_base::keep_alive(true));
                  self->connectHandler_();
                }
              });
          }
        });
    }
    catch (...)
    {
      notifyError(boost::current_exception_diagnostic_information());
    }
  }

  void setSelfSustainingUntilDisconnect(bool selfSustain)
  {
    selfSustainUntilDisconnect_ = selfSustain;
  }

  void disconnect() override
  {
    resetHandler();
    boost::system::error_code ec;
    socket_->get().lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_->get().lowest_layer().close();
  }

  bool isConnected() const override
  {
    boost::system::error_code ec;
    auto remoteEndpoint = socket_->get().lowest_layer().remote_endpoint(ec);
    return !ec && socket_->get().lowest_layer().is_open();
  }

  std::string getConnectedIp() const override
  {
    return isConnected() ? socket_->get().lowest_layer().remote_endpoint().address().to_string() : "";
  }

  uint16_t getConnectedPort() const override
  {
    return isConnected() ? socket_->get().lowest_layer().remote_endpoint().port() : 0;
  }

  void send(const std::string& data) override
  {
    LOG_TRACE << "net::client::BoostConnection<" << getConnectedIp() << ":" << getConnectedPort() << ">::send: " << data;

    auto self = this->shared_from_this();
    boost::asio::async_write(
      socket_->get(), boost::asio::buffer(data.data(), data.size()),
      [self](const boost::system::error_code& error, std::size_t bytes_transferred)
      {
        if (error)
        {
          self->notifyError(error.message());
        }
      });
  }

protected:
  void startReading(const std::string& delimiter) override
  {
    delimiter_ = delimiter;
    triggerRead();
  }

private:
  void triggerRead()
  {
    // captures a shared pointer to keep the connection object alive until it is disconnected
    auto self = this->shared_from_this();
    boost::asio::async_read_until(
        socket_->get(),
        receiveBuffer_,
        delimiter_,
        [self](const boost::system::error_code &error, size_t bytes_transferred)
        {
          if (self)
          {
            if (!error)
            {
              std::string data(
                boost::asio::buffers_begin(self->receiveBuffer_.data()),
                boost::asio::buffers_begin(self->receiveBuffer_.data()) + bytes_transferred);
              self->receiveBuffer_.consume(bytes_transferred);
              LOG_TRACE << "net::client::BoostConnection<" << self->getConnectedIp() << ":"
                        << self->getConnectedPort() << ">::handleRead: " << data;
              self->notifyRead(data);
              self->triggerRead();
            }
            else
            {
              LOG_TRACE << "net::client::BoostConnection<"
                        << self->getConnectedIp() << ":" << self->getConnectedPort()
                        << "> Read failed: " << error.message();
              self->notifyError(error.message());
            }
          }
        });
  }

private:
  Connection::ConnectHandler connectHandler_;
  std::shared_ptr<boost::asio::io_service> ioService_;
  std::shared_ptr<SOCKET> socket_;
  boost::asio::streambuf receiveBuffer_;
  bool selfSustainUntilDisconnect_;
  std::string delimiter_;
};

} //namespace client
} //namespace ses
} //namespace net
