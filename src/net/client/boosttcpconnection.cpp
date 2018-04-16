#include "net/client/boosttcpconnection.hpp"

namespace ses {
namespace net {
namespace client {

BoostTcpConnection::Ptr establishBoostTcpConnection(
    const std::shared_ptr<boost::asio::io_service>& ioService,
    const std::string &host, uint16_t port,
    const Connection::ConnectHandler& connectHandler,
    const Connection::ReceivedDataHandler& receivedDataHandler,
    const Connection::DisconnectHandler& errorHandler)
{
  auto connection = std::make_shared<BoostTcpConnection > (ioService, connectHandler,
                                                                        receivedDataHandler, errorHandler);
  connection->connect(host, port);
  return connection;
}

} //namespace client
} //namespace ses
} //namespace net
