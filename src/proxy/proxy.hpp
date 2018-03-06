#pragma once

#include <list>
#include <memory>
#include <boost/asio/io_service.hpp>

#include "proxy/server.hpp"
#include "proxy/pool.hpp"

namespace ses {
namespace proxy {

class Proxy : public std::enable_shared_from_this<Proxy>
{
public:
  typedef std::shared_ptr<Proxy> Ptr;

public:
  Proxy(const std::shared_ptr<boost::asio::io_service>& ioService);

  void addPool(const Pool::Configuration& configuration);
  void addServer(const Server::Configuration& configuration);

public:
  void handleNewClient(Client::Ptr newClient);

private:
  std::shared_ptr<boost::asio::io_service> ioService_;
  std::list<Pool::Ptr> pools_;
  std::list<Server::Ptr> servers_;
  std::map<boost::uuids::uuid, Client::Ptr> clients_;
};

} // namespace proxy
} // namespace ses
