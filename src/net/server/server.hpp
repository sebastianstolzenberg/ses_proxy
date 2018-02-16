//
// Created by ses on 16.02.18.
//

#ifndef SES_PROXY_SERVER_HPP
#define SES_PROXY_SERVER_HPP

#include <string>
#include <memory>
#include <boost/core/noncopyable.hpp>

#include "net/connectiontype.hpp"
#include "net/server/connection.hpp"

namespace ses {
namespace net {
namespace server {

class ServerHandler
{
public:
  typedef std::shared_ptr<ServerHandler> Ptr;

protected:
  virtual ~ServerHandler() {};

public:
  void handleNewConnection(const Connection::Ptr& connection) = 0;
};

class Server : private boost::noncopyable
{
public:
  typedef std::shared_ptr<Server> Ptr;
public:
  virtual ~Server() {};
};

Server::Ptr createServer(const ServerHandler::Ptr& handler,
                         ConnectionType type,
                         const std::string& address,
                         uint16_t port);


} //namespace server
} //namespace net
} //namespace ses

#endif //SES_PROXY_SERVER_HPP
