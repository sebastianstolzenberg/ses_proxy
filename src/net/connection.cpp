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
