#include <iostream>
#include <thread>
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

  bool send(const std::string& data) override
  {
    LOG_TRACE << "net::server::BoostConnection::send: " << data;

    boost::system::error_code error;
    boost::asio::write(socket_, boost::asio::buffer(data.data(), data.size()), error);
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
    auto self = selfSustainUntilDisconnect_ ?
                std::enable_shared_from_this<BoostConnection<SocketType> >::shared_from_this() :
                BoostConnection::Ptr();
    boost::asio::async_read_until(
        socket_,
        receiveBuffer_,
        '\n',
        [this, self](boost::system::error_code error, size_t bytes_transferred)
        {
          if (!error)
          {
            boost::asio::streambuf::const_buffers_type bufs = receiveBuffer_.data();
            std::string data(boost::asio::buffers_begin(bufs),
                             boost::asio::buffers_begin(bufs) + receiveBuffer_.size());
            receiveBuffer_.consume(receiveBuffer_.size());
            LOG_TRACE << "net::server::BoostConnection received : " << data;
            notifyRead(data);
            triggerRead();
          }
          else
          {
            LOG_TRACE << "net::server::BoostConnection Read failed: " << error.message();
            notifyError(error.message());
          }
        });
  }

private:
  SocketType socket_;
  boost::asio::streambuf receiveBuffer_;
  bool selfSustainUntilDisconnect_;
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
    acceptor_.listen();

    accept();
  }

private:
  virtual void accept()
  {
    auto nextConnection = std::make_shared<BoostConnection<boost::asio::ip::tcp::socket> >(*ioService_);
    acceptor_.async_accept(
      nextConnection->socket().lowest_layer(),
      [this, nextConnection](boost::system::error_code ec)
      {
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open())
        {
          return;
        }

        if (!ec && newConnecionHandler_)
        {
          newConnecionHandler_(nextConnection);
        }

      accept();
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
    auto nextConnection =
      std::make_shared<BoostConnection<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> > >(*ioService_, context_);
    acceptor_.async_accept(
      nextConnection->socket().lowest_layer(),
      [this, nextConnection](boost::system::error_code ec)
      {
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open())
        {
          return;
        }

        accept();

        if (!ec && newConnecionHandler_)
        {
          nextConnection->socket().async_handshake(
            boost::asio::ssl::stream_base::server,
            [this, nextConnection] (const boost::system::error_code& error)
            {
              if (!error && newConnecionHandler_)
              {
                newConnecionHandler_(nextConnection);
              }
            });
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