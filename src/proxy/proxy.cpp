//
// Created by ses on 24.02.18.
//

#include "proxy.hpp"
#include "util/log.hpp"

namespace ses {
namespace proxy {

namespace {
void sortByWeightedWorkers(std::list<Pool>& pools)
{

}
}

Proxy::Proxy(const std::shared_ptr<boost::asio::io_service>& ioService)
  : ioService_(ioService), loadBalancerTimer_(*ioService)
{
}

void Proxy::addPool(const Pool::Configuration& configuration)
{
  auto pool = std::make_shared<Pool>(ioService_);
  pool->connect(configuration);
  pools_.push_back(pool);

  triggerLoadBalancerTimer();
}

void Proxy::addServer(const Server::Configuration& configuration)
{
  auto server = std::make_shared<Server>(ioService_);
  server->start(configuration, std::bind(&Proxy::handleNewClient, this, std::placeholders::_1));
  servers_.push_back(server);
}

void Proxy::handleNewClient(const Client::Ptr& newClient)
{
  if (newClient)
  {
    clients_[newClient->getIdentifier()] = newClient;

    if (!pools_.empty())
    {
      // sorts the pools to find the pool which needs the next miner the most
      pools_.sort([newClient](const auto& a, const auto& b)
                  {
                    if (b->getAlgotrithm() != newClient->getAlgorithm())
                    {
                      return true;
                    }
                    else if (a->getAlgotrithm() != newClient->getAlgorithm())
                    {
                      return false;
                    }
                    float aWeighted = a->weightedWorkers();
                    float bWeighted = b->weightedWorkers();
                    if (aWeighted == bWeighted)
                    {
                      return a->getWeight() > b->getWeight();
                    }
                    else
                    {
                      return aWeighted < bWeighted;
                    }
                  });
      LOG_INFO << "Ordered pools by weighted load:";
      for (auto pool : pools_) LOG_INFO << "  " << pool->getDescriptor()
                                        << ", weightedWorkers, " << pool->weightedWorkers()
                                        << ", weight, " << pool->getWeight()
                                        << ", workers, " << pool->numWorkers()
                                        << ", algorithm, " << pool->getAlgotrithm();
      for (auto pool : pools_)
      {
        if (pool->getAlgotrithm() == newClient->getAlgorithm() &&
            pool->addWorker(newClient))
        {
          newClient->setDisconnectHandler(
              [this, newClient]()
              {
                // removes the client from the pools when they disconnect
                for (auto pool : pools_) pool->removeWorker(newClient);
                clients_.erase(newClient->getIdentifier());
              });
          break;
        }
      }
    }
  }
}

void Proxy::triggerLoadBalancerTimer()
{
  //TODO optimize duration
  loadBalancerTimer_.expires_from_now(boost::posix_time::seconds(30));
  loadBalancerTimer_.async_wait(
    [this](const boost::system::error_code& error)
    {
      if (!error)
      {
        triggerLoadBalancerTimer();
        balancePoolLoads();
      }
    });
}

void Proxy::balancePoolLoads()
{
  //TODO balancing algorithm
  for(auto& pool : pools_)
  {
    LOG_INFO << " >>> balancePoolLoads - " << pool->getDescriptor()
             << ", weightedHashRate, " << pool->weightedHashRate()
             << ", hashRate, " << pool->hashRate()
             << ", weightedWorkers, " << pool->weightedWorkers()
             << ", weight, " << pool->getWeight()
             << ", workers, " << pool->numWorkers()
             << ", algorithm, " << pool->getAlgotrithm();
    for (auto& worker : pool->getWorkersSortedByHashrateDescending())
    {

    }
  }
}

} // namespace proxy
} // namespace ses