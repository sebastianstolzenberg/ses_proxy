#include "net/client/boosttlsconnection.hpp"

namespace ses {
namespace net {
namespace client {

BoostTlsConnection::Ptr establishBoostTlsConnection(
    const std::shared_ptr<boost::asio::io_service>& ioService,
    const std::string &host, uint16_t port,
    const Connection::ConnectHandler& connectHandler,
    const Connection::ReceivedDataHandler& receivedDataHandler,
    const Connection::DisconnectHandler& errorHandler)
{
  auto connection = std::make_shared<BoostTlsConnection> (
      ioService, connectHandler, receivedDataHandler, errorHandler);
  connection->connect(host, port);
  return connection;
}

} //namespace client
} //namespace ses
} //namespace net
