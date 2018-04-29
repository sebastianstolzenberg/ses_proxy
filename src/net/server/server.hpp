#pragma once

#include <string>
#include <memory>
#include <functional>
#include <boost/core/noncopyable.hpp>
#include <boost/asio/io_service.hpp>

#include "net/endpoint.hpp"
#include "net/connection.hpp"

namespace ses {
namespace net {
namespace server {

typedef std::function<void(Connection::Ptr connection)> NewConnectionHandler;

class Server : private boost::noncopyable, public std::enable_shared_from_this<Server>
{
public:
  typedef std::shared_ptr<Server> Ptr;

  virtual void startAccept() = 0;
  virtual void cancelAccept() = 0;

public:
  virtual ~Server() {};
};

Server::Ptr createServer(const std::shared_ptr<boost::asio::io_service>& ioService,
                         const NewConnectionHandler& handler, const EndPoint& endPoint);

} //namespace server
} //namespace net
} //namespace ses
