#pragma once

#include <list>
#include <memory>
#include <boost/asio/io_service.hpp>

#include "proxy/server.hpp"
#include "proxy/pool.hpp"
#include "util/hashratebalancer.hpp"

namespace ses {
namespace proxy {

class Proxy : public std::enable_shared_from_this<Proxy>
{
public:
  typedef std::shared_ptr<Proxy> Ptr;

public:
  Proxy(const std::shared_ptr<boost::asio::io_service>& ioService, uint32_t loadBalanceInterval);

  void addPool(const Pool::Configuration& configuration);
  void addServer(const Server::Configuration& configuration);

public:
  void handleNewClient(const Client::Ptr& newClient);

private:
  void triggerLoadBalancerTimer();
  void balancePoolLoads();

private:
  std::shared_ptr<boost::asio::io_service> ioService_;
  uint32_t loadBalanceInterval_;
  boost::asio::deadline_timer loadBalancerTimer_;
  std::recursive_mutex mutex_;

  std::list<Pool::Ptr> pools_;
  std::list<Server::Ptr> servers_;
  std::map<boost::uuids::uuid, Client::Ptr> clients_;

  util::HashRateCollector<Client> clientsTracker_;
  util::HashRateCollector<Pool> poolsTracker_;
};

} // namespace proxy
} // namespace ses
