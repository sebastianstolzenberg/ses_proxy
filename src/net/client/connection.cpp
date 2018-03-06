/* XMRig
 * Copyright 2018      Sebastian Stolzenberg <https://github.com/sebastianstolzenberg>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <boost/exception/diagnostic_information.hpp>

#include "net/client/connection.hpp"
#include "net/client/boosttlsconnection.hpp"
#include "net/client/boosttcpconnection.hpp"


namespace ses {
namespace net {
namespace client {

Connection::Ptr establishConnection(const std::shared_ptr<boost::asio::io_service>& ioService,
                                    const EndPoint& endPoint,
                                    const Connection::ReceivedDataHandler& receivedDataHandler,
                                    const Connection::ErrorHandler& errorHandler)
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
        connection = establishBoostTlsConnection(ioService, endPoint.host_, endPoint.port_, receivedDataHandler, errorHandler);
        break;

      case CONNECTION_TYPE_TCP:
        connection = establishBoostTcpConnection(ioService, endPoint.host_, endPoint.port_, receivedDataHandler, errorHandler);

      default:
        break;
    }
  }
  catch (...)
  {
    std::cout << boost::current_exception_diagnostic_information();
  }
  return connection;
}

} //namespace client
} //namespace ses
} //namespace net
