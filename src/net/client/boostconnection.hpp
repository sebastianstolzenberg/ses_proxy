#pragma once

#include "net/client/connection.hpp"
#include "util/log.hpp"

namespace ses {
namespace net {
namespace client {

template<class SOCKET>
class BoostConnection : public Connection, public std::enable_shared_from_this<BoostConnection<SOCKET> >
{
public:
  BoostConnection(const std::shared_ptr<boost::asio::io_service>& ioService,
                  const std::string &server, uint16_t port,
                  const Connection::ReceivedDataHandler& receivedDataHandler,
                  const Connection::DisconnectHandler& errorHandler)
    : Connection(receivedDataHandler, errorHandler),
      ioService_(ioService), socket_(*ioService)
  {
    LOG_TRACE << "Connecting net::client::BoostConnection to " << server << ":" << port;
    boost::asio::ip::tcp::resolver resolver(*ioService_);
    boost::asio::ip::tcp::resolver::query query(server, std::to_string(port));
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    socket_.connect(iterator);
    startReading();
  }

  ~BoostConnection()
  {
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

  bool send(const char *data, std::size_t size) override
  {
    LOG_TRACE << "net::client::BoostConnection<" << getConnectedIp() << ":" << getConnectedPort() << ">::send: ";
    LOG_TRACE.write(data, size);

    boost::system::error_code error;
    boost::asio::write(socket_.get(), boost::asio::buffer(data, size), error);
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
    boost::asio::async_read(socket_.get(),
                            boost::asio::buffer(receiveBuffer_, sizeof(receiveBuffer_)),
                            boost::asio::transfer_at_least(1),
                            [this, self](const boost::system::error_code &error,
                                         size_t bytes_transferred)
                            {
                              if (!error)
                              {
                                LOG_TRACE << "net::client::BoostConnection<" << getConnectedIp() << ":"
                                          << getConnectedPort() << ">::handleRead: ";
                                LOG_TRACE.write(receiveBuffer_, bytes_transferred);
                                notifyRead(receiveBuffer_, bytes_transferred);
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
  std::shared_ptr<boost::asio::io_service> ioService_;
  SOCKET socket_;
  char receiveBuffer_[2048];
  bool selfSustainUntilDisconnect_;
};

} //namespace client
} //namespace ses
} //namespace net
