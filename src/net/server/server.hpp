//
// Created by ses on 16.02.18.
//

#ifndef SES_NET_SERVER_SERVER_HPP
#define SES_NET_SERVER_SERVER_HPP

#include <string>
#include <memory>
#include <functional>
#include <boost/core/noncopyable.hpp>

#include "net/endpoint.hpp"
#include "net/connection.hpp"

namespace ses {
namespace net {
namespace server {

typedef std::function<void(Connection::Ptr connection)> NewConnectionHandler;

class Server : private boost::noncopyable
{
public:
  typedef std::shared_ptr<Server> Ptr;

public:
  virtual ~Server() {};
};

Server::Ptr createServer(const NewConnectionHandler& handler, const EndPoint& endPoint);


} //namespace server
} //namespace net
} //namespace ses

#endif //SES_NET_SERVER_SERVER_HPP
