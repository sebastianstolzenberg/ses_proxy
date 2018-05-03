#include <iostream>
#include <thread>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "net/server/server.hpp"
#include "util/log.hpp"

#undef LOG_COMPONENT
#define LOG_COMPONENT net

namespace ses {
namespace net {
namespace server {

template <typename SocketType>
class BoostConnection : public Connection,
                        public std::enable_shared_from_this<BoostConnection<SocketType> >
{
public:
  explicit BoostConnection(boost::asio::io_service& io_service)
    : socket_(io_service), selfSustainUntilDisconnect_(false)
  {
  }

  explicit BoostConnection(boost::asio::io_service& io_service, boost::asio::ssl::context& context)
    : socket_(io_service, context), selfSustainUntilDisconnect_(false)
  {
  }

  ~BoostConnection()
  {
    LOG_INFO << __PRETTY_FUNCTION__;
  }

  SocketType& socket()
  {
    return socket_;
  }

public:
  void setSelfSustainingUntilDisconnect(bool selfSustain) override
  {
    selfSustainUntilDisconnect_ = selfSustain;
  }

  void disconnect() override
  {
    resetHandler();
    boost::system::error_code ec;
    socket_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.lowest_layer().close();
  }

  bool isConnected() const override
  {
    return socket_.lowest_layer().is_open();
  }

  std::string getConnectedIp() const override
  {
    return isConnected() ? socket_.lowest_layer().remote_endpoint().address().to_string() : "";
  }

  uint16_t getConnectedPort() const override
  {
    return isConnected() ? socket_.lowest_layer().remote_endpoint().port() : 0;
  }

  void send(const std::string& data) override
  {
    LOG_TRACE << "net::server::BoostConnection::send: " << data;

    auto self = this->shared_from_this();
    boost::asio::async_write(
      socket_, boost::asio::buffer(data.data(), data.size()),
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
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    // captures a shared pointer to keep the connection object alive until it is disconnected
    auto self = this->shared_from_this();
    boost::asio::async_read_until(
        socket_,
        receiveBuffer_,
        delimiter_,
        [self](boost::system::error_code error, size_t bytes_transferred)
        {
          if (self)
          {
            if (!error)
            {
              std::lock_guard<std::recursive_mutex> lock(self->mutex_);
              std::string data(
                  boost::asio::buffers_begin(self->receiveBuffer_.data()),
                  boost::asio::buffers_begin(self->receiveBuffer_.data()) + bytes_transferred);
              self->receiveBuffer_.consume(bytes_transferred);
              LOG_TRACE << "net::server::BoostConnection<"
                        << self->getConnectedIp() << ":" << self->getConnectedPort()
                        << "> received : " << data;
              self->notifyRead(data);
              self->triggerRead();
            }
            else
            {
              LOG_ERROR << "\"net::server::BoostConnection<"
                        << self->getConnectedIp() << ":" << self->getConnectedPort()
                        << "> Read failed: " << error.message();
              self->notifyError(error.message());
            }
          }
        });
  }

private:
  std::recursive_mutex mutex_;
  SocketType socket_;
  boost::asio::streambuf receiveBuffer_;
  bool selfSustainUntilDisconnect_;
  std::string delimiter_;
};

class BoostServer : public Server
{
public:
  BoostServer(const std::shared_ptr<boost::asio::io_service>& ioService,
              const NewConnectionHandler& handler, const std::string& address, uint16_t port)
    : newConnecionHandler_(handler)
    , ioService_(ioService)
    , acceptor_(*ioService_)
  {
    //TODO signal handling

    boost::asio::ip::tcp::resolver resolver(*ioService_);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({address, std::to_string(port)});
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
  }

  void startAccept() override
  {
    acceptor_.listen();
    accept();
  }

  void cancelAccept() override
  {
    boost::system::error_code error;
    acceptor_.cancel(error);
  }

private:
  virtual void accept()
  {
    auto self = std::static_pointer_cast<BoostServer>(shared_from_this());
    auto nextConnection = std::make_shared<BoostConnection<boost::asio::ip::tcp::socket> >(*ioService_);
    acceptor_.async_accept(
      nextConnection->socket().lowest_layer(),
      [self, nextConnection](boost::system::error_code ec)
      {
        if (self)
        {
          // Check whether the server was stopped by a signal before this
          // completion handler had a chance to run.
          if (!self->acceptor_.is_open())
          {
            return;
          }

          if (!ec)
          {
            self->accept();

            if (self->newConnecionHandler_)
            {
              self->newConnecionHandler_(nextConnection);
            }
          }
        }
      });
  }

protected:
  std::shared_ptr<boost::asio::io_service> ioService_;
  NewConnectionHandler newConnecionHandler_;
  boost::asio::ip::tcp::acceptor acceptor_;
};

class BoostTlsServer : public BoostServer
{
public:
  BoostTlsServer(const std::shared_ptr<boost::asio::io_service>& ioService,
                 const NewConnectionHandler& handler, const std::string& address, uint16_t port,
                 const std::string& certificateChainFile, const std::string& privateKeyFile)
    : BoostServer(ioService, handler, address, port),
      context_(boost::asio::ssl::context::sslv23_server)
  {
    context_.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2);
    context_.use_certificate_chain_file(certificateChainFile);
    context_.use_private_key_file(privateKeyFile, boost::asio::ssl::context::pem);
  }

private:
  virtual void accept()
  {
    auto self = std::static_pointer_cast<BoostTlsServer>(shared_from_this());
    auto nextConnection =
      std::make_shared<BoostConnection<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> > >(*ioService_, context_);
    acceptor_.async_accept(
      nextConnection->socket().lowest_layer(),
      [self, nextConnection](boost::system::error_code ec)
      {
        if (self)
        {
          // Check whether the server was stopped by a signal before this
          // completion handler had a chance to run.
          if (!self->acceptor_.is_open())
          {
            return;
          }

          if (!ec)
          {
            self->accept();

            if (self->newConnecionHandler_)
            {
              nextConnection->socket().async_handshake(
                boost::asio::ssl::stream_base::server,
                [self, nextConnection](const boost::system::error_code& error)
                {
                  if (self && !error && self->newConnecionHandler_)
                  {
                    self->newConnecionHandler_(nextConnection);
                  }
                });
            }
          }
        }
      });
  }

private:
  boost::asio::ssl::context context_;
};

Server::Ptr createServer(const std::shared_ptr<boost::asio::io_service>& ioService,
                         const NewConnectionHandler& handler, const EndPoint& endPoint)
{
  if (endPoint.connectionType_ == net::CONNECTION_TYPE_TCP)
  {
    return std::make_shared<BoostServer>(ioService, handler, endPoint.host_, endPoint.port_);
  }
  else if (endPoint.connectionType_ == net::CONNECTION_TYPE_TLS)
  {
    return std::make_shared<BoostTlsServer>(ioService, handler, endPoint.host_, endPoint.port_,
                                            endPoint.certificateChainFile_, endPoint.privateKeyFile_);
  }
}

} //namespace server
} //namespace net
} //namespace ses