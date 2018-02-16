//
// Created by ses on 16.02.18.
//

#ifndef SES_PROXY_CONNECTION_HPP
#define SES_PROXY_CONNECTION_HPP

#include <string>
#include <memory>
#include <boost/core/noncopyable.hpp>

namespace ses {
namespace net {
namespace server {

class ConnectionHandler
{
public:
  typedef std::shared_ptr<ConnectionHandler> Ptr;
  typedef std::weak_ptr<ConnectionHandler> WeakPtr;

protected:
  virtual ~ConnectionHandler() {}

public:
  virtual void handleReceived(const std::string& data) = 0;
  virtual void handleDisconnected() = 0;

};

class Connection : private boost::noncopyable
{
public:
  typedef std::shared_ptr<Connection> Ptr;

protected:
  Connection() {};

public:
  void send(const std::string& data);
};

} //namespace server
} //namespace net
} //namespace ses

#endif //SES_PROXY_CONNECTION_HPP
