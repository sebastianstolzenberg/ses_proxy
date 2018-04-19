#include <future>
#include <boost/range/numeric.hpp>

#include "proxy.hpp"
#include "util/log.hpp"

#undef LOG_COMPONENT
#define LOG_COMPONENT proxy

namespace ses {
namespace proxy {

namespace {
void sortByWeightedWorkers(std::list<Pool>& pools)
{
}
}

Proxy::Proxy(const std::shared_ptr<boost::asio::io_service>& ioService, uint32_t loadBalanceInterval)
  : ioService_(ioService), loadBalanceInterval_(loadBalanceInterval), loadBalancerTimer_(*ioService)
{
  ccClientStatus_.clientId_ = "ses-proxy";
  ccClientStatus_.version_ = "0.1";
}

void Proxy::addPool(const Pool::Configuration& configuration)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto pool = std::make_shared<Pool>(ioService_);
  pool->connect(configuration);
  pools_.push_back(pool);
//  poolsTracker_.addHasher(pool);

  triggerLoadBalancerTimer();
}

void Proxy::addServer(const Server::Configuration& configuration)
{
  auto server = std::make_shared<Server>(ioService_);
  server->start(configuration, std::bind(&Proxy::handleNewClient, this, std::placeholders::_1));
  servers_.push_back(server);
}

void Proxy::addCcClient(const CcClient::Configuration& configuration)
{
  ccClient_ = std::make_shared<CcClient>(ioService_);
  ccClient_->connect(configuration);
}

void Proxy::handleNewClient(const Client::Ptr& newClient)
{
  if (newClient)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    clients_.push_back(newClient);
//    clientsTracker_.addHasher(newClient);

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
                    double aWeighted = a->weightedHashRate();
                    double bWeighted = b->weightedHashRate();
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
                                        << ", weightedHashRate, " << pool->weightedHashRate()
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
                clients_.remove(newClient);
//                clientsTracker_.removeHasher(newClient);
              });
          break;
        }
      }
    }
  }
}

void Proxy::triggerLoadBalancerTimer()
{
  //TODO optimize timer period
  loadBalancerTimer_.expires_from_now(boost::posix_time::seconds(loadBalanceInterval_));
  loadBalancerTimer_.async_wait(
    [this](const boost::system::error_code& error)
    {
      if (!error)
      {
        triggerLoadBalancerTimer();
        std::thread(std::bind(&Proxy::balancePoolLoads, shared_from_this())).detach();
      }
    });
}

void Proxy::balancePoolLoads()
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  if (pools_.size() > 1 && !clients_.empty())
  {
    util::hashrate::rebalance(clients_, pools_);
  }

  ccClientStatus_.hashRateShort_ = 0;
  ccClientStatus_.hashRateLong_ = 0;
  ccClientStatus_.sharesGood_ = 0;
  ccClientStatus_.sharesTotal_ = 0;
  ccClientStatus_.hashesTotal_ = 0;
  ccClientStatus_.numMiners_ = clients_.size();
  for (auto& pool : pools_)
  {
    auto poolHashRate = pool->hashRate();
    auto poolHashRateAverageLong = pool->getWorkerHashRate().getAverageHashRateLongTimeWindow();
    auto poolTotalSubmitted = pool->getSubmitHashRate().getTotalHashes();

    ccClientStatus_.hashRateShort_ += poolHashRate;
    ccClientStatus_.hashRateLong_ += poolHashRateAverageLong;
    ccClientStatus_.sharesTotal_ += poolTotalSubmitted;

    LOG_WARN << "After rebalance: " << pool->getDescriptor()
             << " , numWorkers, " << pool->numWorkers()
             << " , workersHashRate, " << poolHashRate
             << " , workersHashRateAverage, " << poolHashRateAverageLong
             << " , submitHashRateAverage10min, " << pool->getSubmitHashRate().getAverageHashRateLongTimeWindow()
             << " , submittedHashes, " << poolTotalSubmitted
             << " , weight , " << pool->getWeight();

  }
  ccClientStatus_.sharesGood_ = ccClientStatus_.sharesTotal_;
  ccClientStatus_.hashRateHighest_ = std::max(ccClientStatus_.hashRateHighest_, ccClientStatus_.hashRateShort_);

  ccClient_->publishStatus(ccClientStatus_);
}
//
////  clientsTracker_.sampleCurrentState();
////  LOG_ERROR << "clientsTracker: accumulatedHashRate, " << clientsTracker_.accumulatedHashRate_;
////  poolsTracker_.sampleCurrentState();
////  LOG_ERROR << "poolsTracker: accumulatedHashRate, " << poolsTracker_.accumulatedHashRate_;
//
//  // sort pools by weighted hashrate
//  typedef std::map<uint32_t, Pool::Ptr> PoolsByWeightedHashrate;
//  PoolsByWeightedHashrate poolsSortedByWeightedHashrate;
//
//  // calculates average weighted hashrate
//  uint32_t totalWorkers = 0;
//  uint32_t totalHashrate = 0;
//  uint32_t averageWeightedHashrate = 0;
//  uint32_t numPools = 0;
//  for(auto& pool : pools_)
//  {
//    ++numPools;
//    const auto& hashRate = pool->getSubmitHashRate();
//    totalHashrate += hashRate.getAverageHashRateLongTimeWindow();
//    uint32_t weightedHashRate = hashRate.getAverageHashRateLongTimeWindow() / pool->getWeight();
//    averageWeightedHashrate += weightedHashRate;
//    poolsSortedByWeightedHashrate[weightedHashRate] = pool;
//    totalWorkers += pool->numWorkers();
//    LOG_INFO << "Balance inspection: " << pool->getDescriptor()
//             << ", hashRate, " << hashRate.getAverageHashRateLongTimeWindow()
//             << ", weightedHashRate, " << weightedHashRate
//             << ", weight, " << pool->getWeight()
//             << ", workers, " << pool->numWorkers()
//             << ", algorithm, " << pool->getAlgotrithm();
//  }
//
//  averageWeightedHashrate /= numPools;
//  LOG_WARN << "Balancing loads of " << numPools << " pools, totalHashrate, " << totalHashrate
//           << ", averageWeightedHashrate, " << averageWeightedHashrate
//           << ", totalWorkers, " << totalWorkers;
//
//  //TODO const uint32_t allowedHashrateHystersis = 100;
//
//  // remove excessive hashrate from pools with too high load
//  typedef std::multimap<int32_t, Worker::Ptr> AvailableWorkersByHashrate;
//  AvailableWorkersByHashrate availableWorkersByHashrate;
//  std::mutex availableWorkersMutex;
//  typedef std::packaged_task<void(Pool::Ptr&, uint32_t)> RemoveExcessiveHashrateTask;
//  std::list<std::future<void> > removeExcessiveHashrateTasks;
//  for(auto& pool : poolsSortedByWeightedHashrate)
//  {
//    if (pool.first > averageWeightedHashrate)
//    {
//      LOG_DEBUG << "balancePoolLoads() trying to remove workers from pool " << pool.second->getDescriptor() << " with hashRate " << pool.first;
//      std::shared_ptr<RemoveExcessiveHashrateTask> task =
//        std::make_shared<RemoveExcessiveHashrateTask>(
//          [&availableWorkersByHashrate, &availableWorkersMutex](Pool::Ptr& pool, int32_t excessiveWeightedHashrate)
//          {
//            excessiveWeightedHashrate *= pool->getWeight();
//            auto workersSortedByHashrateDescending =  pool->getWorkersSortedByHashrateDescending();
//            Worker::Ptr lastWorkerWithNonZeroHashrate;
//            for (auto& worker : workersSortedByHashrateDescending)
//            {
//              auto hashRate = worker->getSubmitHashRate().getAverageHashRateLongTimeWindow();
//              if (hashRate > 0)
//              {
//                if (hashRate <= excessiveWeightedHashrate)
//                {
//                  if (pool->removeWorker(worker))
//                  {
//                    excessiveWeightedHashrate -= hashRate;
//
//                    std::lock_guard<std::mutex> lock(availableWorkersMutex);
//                    availableWorkersByHashrate.insert(AvailableWorkersByHashrate::value_type(-hashRate, worker));
//                  }
//                }
//                else
//                {
//                  lastWorkerWithNonZeroHashrate = worker;
//                }
//              }
//              if (excessiveWeightedHashrate <= 0)
//              {
//                break;
//              }
//            }
//            // If there is still too much hash rate assigned to the pool, also takes the slowest worker (with hashrate > 0).
//            // This will definitely push the excessiveWeightedHashrate below zero, otherwise the worker would
//            // have been picked by the loop above, already.
//            if (excessiveWeightedHashrate > 0 && lastWorkerWithNonZeroHashrate)
//            {
//              auto hashRate = lastWorkerWithNonZeroHashrate->getSubmitHashRate().getAverageHashRateLongTimeWindow();
//              if (pool->removeWorker(lastWorkerWithNonZeroHashrate))
//              {
//                excessiveWeightedHashrate -= hashRate;
//                std::lock_guard<std::mutex> lock(availableWorkersMutex);
//                availableWorkersByHashrate.insert(
//                  AvailableWorkersByHashrate::value_type(-hashRate, lastWorkerWithNonZeroHashrate));
//              }
//            }
//          });
//      ioService_->post(std::bind(&RemoveExcessiveHashrateTask::operator(), task, pool.second, (pool.first - averageWeightedHashrate)));
//      removeExcessiveHashrateTasks.push_back(std::move(task->get_future()));
//    }
//  }
//  // waits for completion of the started tasks
//  for (auto& task : removeExcessiveHashrateTasks) { task.wait(); }
//
//  // TODO special treatment of pools without workers
//
//  // adds available workers to underloaded pools
//  auto lowHashratePoolsIterator = poolsSortedByWeightedHashrate.begin();
//  while (!availableWorkersByHashrate.empty() &&
//         lowHashratePoolsIterator != poolsSortedByWeightedHashrate.end() &&
//         lowHashratePoolsIterator->first < averageWeightedHashrate)
//  {
//    auto& pool = lowHashratePoolsIterator->second;
//    auto hashRate = lowHashratePoolsIterator->first;
//    uint32_t hashrateDeficit = averageWeightedHashrate - hashRate;
//    LOG_DEBUG << "balancePoolLoads() trying to add workers to pool " << pool->getDescriptor()
//              << " with hashRate " << hashRate << " (hashRateDeficit = " << hashrateDeficit << ")";
//
//    // iterate through available workers, starting with highest hashrates
//    auto availableWorkerIterator = availableWorkersByHashrate.begin();
//    while (availableWorkerIterator != availableWorkersByHashrate.end()
//           // TODO abort condition based on hashrateDeficit threshold -> donation
//          )
//    {
//      int32_t workerHashrate = availableWorkerIterator->first;
//      auto& worker = availableWorkerIterator->second;
//      if (-workerHashrate < hashrateDeficit && pool->addWorker(worker))
//      {
//        hashrateDeficit += workerHashrate;
//        availableWorkerIterator = availableWorkersByHashrate.erase(availableWorkerIterator);
//      }
//      else
//      {
//        availableWorkerIterator++;
//      }
//    }
//    lowHashratePoolsIterator++;
//  }
//
//  if (!availableWorkersByHashrate.empty())
//  {
//    LOG_WARN << "balancePoolLoads() unassigned workers " << availableWorkersByHashrate.size() << ", assigning to slowest pools";
//    for (auto& worker : availableWorkersByHashrate)
//    {
//      for (auto& pool : poolsSortedByWeightedHashrate)
//      {
//        if (pool.second->addWorker(worker.second))
//        {
//          break;
//        }
//      }
//    }
//  }
//}

} // namespace proxy
} // namespace ses