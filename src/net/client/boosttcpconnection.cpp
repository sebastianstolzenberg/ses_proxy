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
#include <thread>

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "net/client/boostconnection.hpp"
#include "net/client/boosttcpconnection.hpp"
#include "util/log.hpp"

namespace ses {
namespace net {
namespace client {

class BoostTcpSocket
{
public:
  typedef boost::asio::ip::tcp::socket SocketType;
public:
  BoostTcpSocket(boost::asio::io_service &ioService)
    : socket_(ioService)
  {
  }

  template<class ITERATOR>
  void connect(ITERATOR &iterator)
  {
    boost::asio::connect(socket_, iterator);
    socket_.set_option(boost::asio::ip::tcp::no_delay(true));
    socket_.set_option(boost::asio::socket_base::keep_alive(true));
  }

  SocketType &get()
  {
    return socket_;
  }

  const SocketType &get() const
  {
    return socket_;
  }

private:
  SocketType socket_;
};

Connection::Ptr establishBoostTcpConnection(const std::shared_ptr<boost::asio::io_service>& ioService,
                                            const std::string &host, uint16_t port,
                                            const Connection::ConnectHandler& connectHandler,
                                            const Connection::ReceivedDataHandler& receivedDataHandler,
                                            const Connection::DisconnectHandler& errorHandler)
{
  auto connection = std::make_shared<BoostConnection<BoostTcpSocket> > (ioService, connectHandler, receivedDataHandler, errorHandler);
  ioService->post(std::bind(&BoostConnection<BoostTcpSocket>::connect, connection, host, port));
  return connection;
}

} //namespace client
} //namespace ses
} //namespace net
