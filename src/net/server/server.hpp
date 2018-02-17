//
// Created by ses on 16.02.18.
//

#ifndef SES_NET_SERVER_SERVER_HPP
#define SES_NET_SERVER_SERVER_HPP

#include <string>
#include <memory>
#include <boost/core/noncopyable.hpp>

#include "net/connectiontype.hpp"
#include "net/connection.hpp"

namespace ses {
namespace net {
namespace server {

class ServerHandler
{
public:
  typedef std::shared_ptr<ServerHandler> Ptr;
  typedef std::weak_ptr<ServerHandler> WeakPtr;

protected:
  virtual ~ServerHandler() {};

public:
  virtual void handleNewConnection(const Connection::Ptr& connection) = 0;
};

class Server : private boost::noncopyable
{
public:
  typedef std::shared_ptr<Server> Ptr;

public:
  virtual ~Server() {};
};

Server::Ptr createServer(const ServerHandler::Ptr& handler,
                         const std::string& address,
                         uint16_t port,
                         ConnectionType type = CONNECTION_TYPE_AUTO);


} //namespace server
} //namespace net
} //namespace ses

#endif //SES_NET_SERVER_SERVER_HPP
