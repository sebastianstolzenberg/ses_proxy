#include <iostream>

#include "net/client/connection.hpp"
#include "net/client/boosttlsconnection.hpp"
#include "net/client/boosttcpconnection.hpp"
#include "util/log.hpp"

namespace ses {
namespace net {
namespace client {

Connection::Ptr establishConnection(const std::shared_ptr<boost::asio::io_service>& ioService,
                                    const EndPoint& endPoint,
                                    const Connection::ConnectHandler& connectHandler,
                                    const Connection::ReceivedDataHandler& receivedDataHandler,
                                    const Connection::DisconnectHandler& errorHandler)
{
  Connection::Ptr connection;

  ConnectionType connectionType = endPoint.connectionType_;
  if (connectionType == CONNECTION_TYPE_AUTO)
  {
    connectionType = (endPoint.port_ == 443) ? CONNECTION_TYPE_TLS : CONNECTION_TYPE_TCP;
  }

  try
  {
    switch (connectionType)
    {
      case CONNECTION_TYPE_TLS:
        connection = establishBoostTlsConnection(ioService, endPoint.host_, endPoint.port_,
                                                 connectHandler, receivedDataHandler, errorHandler);
        break;

      case CONNECTION_TYPE_TCP:
        connection = establishBoostTcpConnection(ioService, endPoint.host_, endPoint.port_,
                                                 connectHandler, receivedDataHandler, errorHandler);

      default:
        break;
    }
  }
  catch (...)
  {
    LOG_CURRENT_EXCEPTION;
  }
  return connection;
}

} //namespace client
} //namespace ses
} //namespace net
