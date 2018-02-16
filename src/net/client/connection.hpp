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

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <cstdint>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>

#include "net/connectiontype.hpp"

namespace ses {
namespace net {
namespace client {

class ConnectionListener
{
public:
  typedef std::shared_ptr<ConnectionListener> Ptr;
  typedef std::weak_ptr<ConnectionListener> WeakPtr;

protected:
  ConnectionListener()
  {};

  virtual ~ConnectionListener()
  {};

public:
  virtual void onReceived(char *data, std::size_t size) = 0;

  virtual void onError(const std::string &error) = 0;
};

class Connection : private boost::noncopyable
{
public:
  typedef std::shared_ptr<Connection> Ptr;

protected:
  Connection(const ConnectionListener::Ptr &listener);

  virtual ~Connection()
  {};

  void notifyRead(char *data, size_t size);

  void notifyError(const std::string &error);

public:
  virtual bool connected() const = 0;

  virtual std::string connectedIp() const = 0;

  virtual bool send(const char *data, std::size_t size) = 0;

private:
  ConnectionListener::WeakPtr listener_;
};

Connection::Ptr establishConnection(const ConnectionListener::Ptr &listener,
                                    ConnectionType type,
                                    const std::string &host, uint16_t port);

} //namespace client
} //namespace net
} //namespace ses

#endif /* __CONNECTION_H__ */
