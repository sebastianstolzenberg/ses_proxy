#pragma once

#include <boost/asio/read_until.hpp>
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
  BoostConnection(const std::shared_ptr<boost::asio::io_service>& ioService,
                  const Connection::ConnectHandler& connectHandler,
                  const Connection::ReceivedDataHandler& receivedDataHandler,
                  const Connection::DisconnectHandler& errorHandler)
    : Connection(receivedDataHandler, errorHandler), connectHandler_(connectHandler),
      ioService_(ioService), socket_(*ioService)
  {
  }

  ~BoostConnection()
  {
  }

  void connect(const std::string &server, uint16_t port)
  {
    try
    {
      LOG_TRACE << "Connecting net::client::BoostConnection to " << server << ":" << port;
      boost::asio::ip::tcp::resolver resolver(*ioService_);
      boost::asio::ip::tcp::resolver::query query(server, std::to_string(port));
      boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

      socket_.connect(iterator);
      startReading();
      connectHandler_();
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
    socket_.get().lowest_layer().close();
  }

  bool isConnected() const override
  {
    return socket_.get().lowest_layer().is_open();
  }

  std::string getConnectedIp() const override
  {
    return isConnected() ? socket_.get().lowest_layer().remote_endpoint().address().to_string() : "";
  }

  uint16_t getConnectedPort() const override
  {
    return isConnected() ? socket_.get().lowest_layer().remote_endpoint().port() : 0;
  }

  bool send(const std::string& data) override
  {
    LOG_TRACE << "net::client::BoostConnection<" << getConnectedIp() << ":" << getConnectedPort() << ">::send: " << data;

    boost::system::error_code error;
    boost::asio::write(socket_.get(), boost::asio::buffer(data.data(), data.size()), error);
    if (error)
    {
      notifyError(error.message());
    }
    return !error;
  }

protected:
  void startReading() override
  {
    triggerRead();
  }

private:
  void triggerRead()
  {
    // captures a shared pointer to keep the connection object alive until it is disconnected
    auto self = selfSustainUntilDisconnect_ ? this->shared_from_this() : BoostConnection<SOCKET>::Ptr();
    boost::asio::async_read_until(
        socket_.get(),
        receiveBuffer_,
        '\n',
        [this, self](const boost::system::error_code &error,
                     size_t bytes_transferred)
        {
          if (!error)
          {
            std::string data(
              boost::asio::buffers_begin(receiveBuffer_.data()),
              boost::asio::buffers_begin(receiveBuffer_.data()) + bytes_transferred);
            receiveBuffer_.consume(bytes_transferred);
            LOG_TRACE << "net::client::BoostConnection<" << getConnectedIp() << ":"
                      << getConnectedPort() << ">::handleRead: " << data;
            notifyRead(data);
            triggerRead();
          }
          else
          {
            LOG_ERROR << "<" << getConnectedIp() << ":" << getConnectedPort() << "> Read failed: "
                      << error.message();
            notifyError(error.message());
          }
        });
  }

private:
  Connection::ConnectHandler connectHandler_;
  std::shared_ptr<boost::asio::io_service> ioService_;
  SOCKET socket_;
  boost::asio::streambuf receiveBuffer_;
  bool selfSustainUntilDisconnect_;
};

} //namespace client
} //namespace ses
} //namespace net
