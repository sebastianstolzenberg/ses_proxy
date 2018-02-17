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

#ifndef __SES_NET_CONNECTION_H__
#define __SES_NET_CONNECTION_H__

#include <cstdint>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>

#include "net/connectiontype.hpp"

namespace ses {
namespace net {

class ConnectionHandler
{
public:
  typedef std::shared_ptr<ConnectionHandler> Ptr;
  typedef std::weak_ptr<ConnectionHandler> WeakPtr;

protected:
  ConnectionHandler()
  {};

  virtual ~ConnectionHandler()
  {};

public:
  virtual void handleReceived(char* data, std::size_t size) = 0;

  virtual void handleError(const std::string& error) = 0;
};

class Connection : private boost::noncopyable
{
public:
  typedef std::shared_ptr<Connection> Ptr;

protected:
  Connection();

  Connection(const ConnectionHandler::Ptr& listener);

  virtual ~Connection()
  {};

  void notifyRead(char* data, size_t size);

  void notifyError(const std::string& error);

public:
  void setHandler(const ConnectionHandler::Ptr& handler);

  virtual bool connected() const = 0;

  virtual std::string connectedIp() const = 0;

  virtual bool send(const char* data, std::size_t size) = 0;
  bool send(const std::string& data) {return send(data.data(), data.size());}

private:
  ConnectionHandler::WeakPtr handler_;
};

} //namespace net
} //namespace ses

#endif /* __SES_NET_CONNECTION_H__ */
