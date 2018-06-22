#include <sstream>
#include <iomanip>
#include <future>
#include <boost/range/numeric.hpp>
#include <boost/asio/signal_set.hpp>

#include "proxy/configurationfile.hpp"
#include "proxy.hpp"
#include "util/log.hpp"

#undef LOG_COMPONENT
#define LOG_COMPONENT proxy

namespace ses {
namespace proxy {

namespace {
void configureLogging(uint32_t logLevel, bool syslog)
{
  if (logLevel == 0)
  {
    boost::log::core::get()->set_logging_enabled(false);
  }
  else
  {
    logLevel = std::min(logLevel, UINT32_C(6));
    boost::log::trivial::severity_level level = boost::log::trivial::severity_level::fatal;
    switch (logLevel)
    {
      case 2:
        level = boost::log::trivial::severity_level::error;
        break;
      case 3:
        level = boost::log::trivial::severity_level::warning;
        break;
      case 4:
        level = boost::log::trivial::severity_level::info;
        break;
      case 5:
        level = boost::log::trivial::severity_level::debug;
        break;
      case 6:
        level = boost::log::trivial::severity_level::trace;
        break;
      case 1: // fall through
      default:
        level = boost::log::trivial::severity_level::fatal;
        break;
    }
    ses::log::initialize(level, syslog);
  }
}

void waitForSignal(boost::asio::io_service& ioService, size_t numThreads)
{
  // waits for signals ending program

  boost::asio::signal_set signals(ioService);
  signals.add(SIGINT);
  signals.add(SIGTERM);
  signals.add(SIGQUIT);
  signals.async_wait(
    [&](boost::system::error_code /*ec*/, int signo)
    {
      LOG_WARN << "Signal " << signo << " received ... exiting";
      ioService.stop();
    });

  if (numThreads == 0)
  {
    numThreads = std::thread::hardware_concurrency();
  }
  LOG_DEBUG << "Launching " << numThreads << " worker threads.";
  // runs io_service on a reasonable number of threads
  for (uint32_t threadCount = 1; threadCount < numThreads; ++threadCount)
  {
    std::thread thread(
      std::bind(static_cast<size_t (boost::asio::io_service::*)()>(&boost::asio::io_service::run),
                &ioService));
    thread.detach();
  }
  // this run call uses the current thread
  ioService.run();
}
}

Proxy::Proxy(const std::shared_ptr<boost::asio::io_service>& ioService, const std::string& configurationFilePath)
  : ioService_(ioService), configurationFilePath_(configurationFilePath), loadBalancerTimer_(*ioService)
{
  ccProxyStatus_.version_ = "0.1";
}

void Proxy::run()
{
  waitForSignal(*ioService_, numThreads_);
}

void Proxy::reloadConfiguration()
{
  // tears down old connections
  boost::system::error_code error;
  loadBalancerTimer_.cancel(error);

  servers_.clear();

  for (auto& client : clients_)
  {
    client->disconnect();
  }
  clients_.clear();

  for (auto& poolGroup : poolGroups_)
  {
    for (auto& pool : poolGroup.second.pools_)
    {
      pool->disconnect();
    }
  }
  poolGroups_.clear();

  ccClient_.reset();


  // parses new configuration
  ses::proxy::Configuration configuration = ses::proxy::parseConfigurationFile(configurationFilePath_);


  // sets up new connections
  configureLogging(configuration.logLevel_, false);

  numThreads_ = configuration.threads_;
  loadBalanceInterval_ = configuration.poolLoadBalanceIntervalSeconds_;

  for (const auto& poolGroupConfig : configuration.poolGroups_)
  {
    auto& poolGroup = poolGroups_[poolGroupConfig.priority_];
    // appends the name, just in case the same priority has been configured for more than one group
    poolGroup.name_ += poolGroupConfig.name_;

    for (const auto& poolConfig : poolGroupConfig.pools_)
    {
      if (poolConfig.weight_ != 0)
      {
        addPool(poolConfig, poolGroup.pools_);
      }
    }
  }

  for (const auto& serverConfig : configuration.server_)
  {
    addServer(serverConfig);
  }

  if (configuration.ccCient_)
  {
    addCcClient(*configuration.ccCient_);
  }

}

void Proxy::addPool(const Pool::Configuration& configuration, std::list<Pool::Ptr>& pools)
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  auto pool = std::make_shared<Pool>(ioService_);
  pool->connect(configuration);
  pools.push_back(pool);

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
  ccClient_ = std::make_shared<CcClient>(ioService_, configuration);
  ccProxyStatus_.clientId_ = configuration.userAgent_;
}

void Proxy::handleNewClient(const Client::Ptr& newClient)
{
  if (newClient)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    clients_.push_back(newClient);
//    clientsTracker_.addHasher(newClient);

    for (auto& poolGroup : poolGroups_)
    {
      if (!poolGroup.second.pools_.empty())
      {
        // sorts the pools to find the pool which needs the next miner the most
        poolGroup.second.pools_.sort([newClient](const auto& a, const auto& b)
                    {
                      if (!newClient->supports(b->getAlgorithm()))
                      {
                        return true;
                      }
                      else if (!newClient->supports(a->getAlgorithm()))
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
        for (auto pool : poolGroup.second.pools_)
          LOG_INFO << "  " << pool->getDescriptor()
                   << ", weightedHashRate, " << pool->weightedHashRate()
                   << ", weight, " << pool->getWeight()
                   << ", workers, " << pool->numWorkers()
                   << ", algorithm, " << pool->getAlgorithm();

        for (auto pool : poolGroup.second.pools_)
        {
          if (pool->addWorker(newClient))
          {
            auto self = shared_from_this();
            newClient->setDisconnectHandler(
                [self, newClient]()
                {
                  // removes the client from the pools when they disconnect
                  for (auto& poolGroup : self->poolGroups_)
                  {
                    for (auto& pool : poolGroup.second.pools_)
                    {
                      pool->removeWorker(newClient);
                    }
                  }
                  self->clients_.remove(newClient);
                });

            // worker has been assigned to a pool
            return;
          }
        }
      }
    }
  }
}

void Proxy::triggerLoadBalancerTimer()
{
  //TODO optimize timer period
  loadBalancerTimer_.expires_from_now(boost::posix_time::seconds(loadBalanceInterval_));
  auto self = shared_from_this();
  loadBalancerTimer_.async_wait(
    [self](const boost::system::error_code& error)
    {
      if (!error)
      {
        self->triggerLoadBalancerTimer();
        std::thread(std::bind(&Proxy::balancePoolLoads, self)).detach();
      }
    });
}

void Proxy::balancePoolLoads()
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  std::list<Client::Ptr> remainingClients = clients_;
  for (auto& poolGroup : poolGroups_)
  {
    if (poolGroup.second.pools_.size() > 1 && !remainingClients.empty())
    {
      // TODO check if at least on of the pools in the group is connected
      remainingClients = util::hashrate::rebalance(remainingClients, poolGroup.second.pools_);
    }
    else
    {
      // pool group cannot accept clients, continues with next group
      continue;
    }

    size_t poolNumber = 0;
    for (auto& pool : poolGroup.second.pools_)
    {
      auto numPoolWorkers = pool->numWorkers();
      auto poolHashRate = pool->hashRate();
      auto
          poolHashRateAverageMedium = pool->getWorkerHashRate().getAverageHashRateShortTimeWindow();
      auto poolHashRateAverageLong = pool->getWorkerHashRate().getAverageHashRateLongTimeWindow();
      auto poolTotalHashes = pool->getSubmitHashRate().getTotalHashes();

      if (ccClient_)
      {
        CcClient::Status ccPoolStatus = pool->getCcStatus();
        std::ostringstream clientId;
        clientId << ccProxyStatus_.clientId_ << "_Pool_" << std::setw(2) << std::setfill('0')
                 << poolNumber;
        ccPoolStatus.clientId_ = clientId.str();
        ++poolNumber;
        ccClient_->publishStatus(ccPoolStatus);
      }

      LOG_WARN << "After rebalance: "
               << poolGroup.second.name_ << ":" << pool->getDescriptor()
               << " , numWorkers, " << numPoolWorkers
               << " , workersHashRate, " << poolHashRate
               << " , workersHashRateAverage, " << poolHashRateAverageLong
               << " , submitHashRateAverage10min, "
               << pool->getSubmitHashRate().getAverageHashRateLongTimeWindow()
               << " , submittedHashes, " << poolTotalHashes
               << " , weight , " << pool->getWeight();
    }
  }

  if (ccClient_)
  {
    ccClient_->send();
  }
}

} // namespace proxy
} // namespace ses