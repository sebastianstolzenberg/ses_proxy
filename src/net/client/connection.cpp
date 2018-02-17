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

Connection::Ptr establishConnection(const ConnectionHandler::Ptr &listener, const std::string &host, uint16_t port,
                                    ConnectionType type)
{
  Connection::Ptr connection;

  if (type == CONNECTION_TYPE_AUTO)
  {
    type = (port == 443) ? CONNECTION_TYPE_TLS : CONNECTION_TYPE_TCP;
  }

  try
  {
    switch (type)
    {
      case CONNECTION_TYPE_TLS:
        connection = establishBoostTlsConnection(listener, host, port);
        break;

      case CONNECTION_TYPE_TCP:
        connection = establishBoostTcpConnection(listener, host, port);

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
