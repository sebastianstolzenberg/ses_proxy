#include <iostream>
#include <boost/exception/diagnostic_information.hpp>

#include "net/client/connection.hpp"
#include "net/client/boosttlsconnection.hpp"
#include "net/client/boosttcpconnection.hpp"


namespace ses {
namespace net {

Connection::Connection(const Connection::ReceivedDataHandler& receivedDataHandler,
                       const Connection::DisconnectHandler& errorHandler)
  : receivedDataHandler_(receivedDataHandler)
  , errorHandler_(errorHandler)
{
}

void Connection::setHandler(const ReceivedDataHandler& receivedDataHandler,
                            const DisconnectHandler& errorHandler)
{
  receivedDataHandler_ = receivedDataHandler;
  errorHandler_ = errorHandler;
  startReading();
}

void Connection::resetHandler()
{
  setHandler(ReceivedDataHandler(), DisconnectHandler());
}

void Connection::notifyRead(const std::string& data)
{
  if (receivedDataHandler_)
  {
    receivedDataHandler_(data);
  }
}

void Connection::notifyError(const std::string &error)
{
  if (errorHandler_)
  {
    errorHandler_(error);
  }
}

} //namespace ses
} //namespace net
