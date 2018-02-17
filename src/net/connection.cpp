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

Connection::Connection()
{
}

Connection::Connection(const ConnectionHandler::Ptr &handler)
  : handler_(handler)
{
}

void Connection::setHandler(const ConnectionHandler::Ptr &handler)
{
  handler_ = handler;
}

void Connection::notifyRead(char *data, size_t size)
{
  ConnectionHandler::Ptr handler = handler_.lock();
  if (handler)
  {
    handler->handleReceived(data, size);
  }
}

void Connection::notifyError(const std::string &error)
{
  ConnectionHandler::Ptr handler = handler_.lock();
  if (handler)
  {
    handler->handleError(error);
  }
}

} //namespace ses
} //namespace net
