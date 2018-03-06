//
// Created by ses on 24.02.18.
//

#include "proxy.hpp"

namespace ses {
namespace proxy {

Proxy::Proxy(const std::shared_ptr<boost::asio::io_service>& ioService)
  : ioService_(ioService)
{
}

void Proxy::addPool(const Pool::Configuration& configuration)
{
  auto pool = std::make_shared<Pool>(ioService_);
  pool->connect(configuration);
  pools_.push_back(pool);
}

void Proxy::addServer(const Server::Configuration& configuration)
{
  auto server = std::make_shared<Server>(ioService_);
  server->start(configuration, std::bind(&Proxy::handleNewClient, this, std::placeholders::_1));
  servers_.push_back(server);
}

void Proxy::handleNewClient(Client::Ptr newClient)
{
  if (newClient)
  {
    clients_[newClient->getIdentifier()] = newClient;
    if (!pools_.empty())
    {
      pools_.front()->addWorker(newClient);
    }
  }
}

} // namespace proxy
} // namespace ses