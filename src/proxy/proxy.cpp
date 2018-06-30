#include <sstream>
#include <iomanip>
#include <future>
#include <boost/range/numeric.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/lexical_cast.hpp>

#include "proxy/configurationfile.hpp"
#include "proxy.hpp"
#include "util/log.hpp"

#undef LOG_COMPONENT
#define LOG_COMPONENT proxy

namespace ses {
namespace proxy {

namespace {
void setLogLevel(uint32_t logLevel)
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
    ses::log::setLogLevel(level);
  }
}

void configureLogging(uint32_t logLevel, bool syslog)
{
  ses::log::initialize(syslog);
  setLogLevel(logLevel);
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
  : ioService_(ioService), configurationFilePath_(configurationFilePath), loadBalancerTimer_(*ioService),
    shell_(std::make_shared<util::shell::Shell>(ioService))
{
  ccProxyStatus_.version_ = "0.1";
  setupShell();
}

void Proxy::run()
{
  shell_->start();
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
#ifdef CC_SUPPORT
  ccClient_ = std::make_shared<CcClient>(ioService_, configuration);
  ccProxyStatus_.clientId_ = configuration.userAgent_;
#endif
}

void Proxy::handleNewClient(const Client::Ptr& newClient)
{
  if (newClient)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    newClients_.push_back(newClient);
    newClient->setNeedsJobHandler(std::bind(&Proxy::handleClientNeedsJob, this, std::placeholders::_1));
  }
}

void Proxy::handleClientNeedsJob(const Client::Ptr& client)
{
  if (client)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    for (auto& poolGroup : poolGroups_)
    {
      if (!poolGroup.second.pools_.empty())
      {
        // sorts the pools to find the pool which needs the next miner the most
        poolGroup.second.pools_.sort([client](const auto& a, const auto& b)
                                     {
                                       if (!client->supports(b->getAlgorithm()))
                                       {
                                         return true;
                                       }
                                       else if (!client->supports(a->getAlgorithm()))
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
          if (pool->addWorker(client))
          {
            newClients_.remove(client);
            clients_.push_back(client);

            auto self = shared_from_this();
            client->setDisconnectHandler(
              [self, client]()
              {
                // removes the client from the pools when they disconnect
                for (auto& poolGroup : self->poolGroups_)
                {
                  for (auto& pool : poolGroup.second.pools_)
                  {
                    pool->removeWorker(client);
                  }
                }
                self->clients_.remove(client);
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

#ifdef CC_SUPPORT
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
#endif

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

#ifdef CC_SUPPORT
  if (ccClient_)
  {
    ccClient_->send();
  }
#endif
}

void Proxy::setupShell()
{
  shell_->addCommand(util::shell::Command("quit",
                                          [this] (const util::shell::Command::Parameters&) {ioService_->stop();},
                                          "Terminates the proxy."));

  shell_->addCommand(
    util::shell::Command(
      "log",
      [](const util::shell::Command::Parameters& parameter)
      {
        uint32_t logLevel;
        if (parameter.size() > 0 && boost::conversion::try_lexical_convert(parameter[0], logLevel))
        {
          std::cout << "Changing log level to " << logLevel << std::endl;
          setLogLevel(logLevel);
        }
      },
      "Changes the log level to the value given as first parameter. "
      "(0 - off, 1 - fatal, 2 - error, 3 - warning, 4 - info, 5 - debug, 6 - trace"));

  shell_->addCommand(util::shell::Command("status", std::bind(&Proxy::printProxyStatus, this),
                                          "Prints the current status of the proxy."));
  shell_->addCommand(util::shell::Command("pools", std::bind(&Proxy::printPoolsStatus, this),
                                          "Lists the configured pools and their current status."));
  shell_->addCommand(util::shell::Command("miner", std::bind(&Proxy::printMinerStatus, this),
                                          "Lists information about connected miners."));

}

void Proxy::printProxyStatus()
{
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  size_t numPoolGroups = poolGroups_.size();
  size_t numPools = 0;
  size_t numMiner = clients_.size();
  size_t hashRate = 0;

  for (auto& poolGroup : poolGroups_)
  {
    numPools += poolGroup.second.pools_.size();
    for (auto& pool : poolGroup.second.pools_)
    {
      hashRate += pool->hashRate();
    }
  }
  std::cout << std::endl
            << "Proxy status:"
            << " hashRate=" << hashRate
            << " numPoolGroups=" << numPoolGroups
            << " numPools=" << numPoolGroups
            << " numMiner=" << numMiner
            << std::endl << std::endl;
}

void Proxy::printPoolsStatus()
{
  std::ostringstream out;

  out << std::endl;
  for (auto& poolGroup : poolGroups_)
  {
    out << std::endl << " Pool group \"" << poolGroup.second.name_ << "\" (priority " << poolGroup.first << ")" << std::endl;
    for (auto& pool : poolGroup.second.pools_)
    {
      out << "   Pool \"" << pool->getDescriptor() << "\" (" << pool->numWorkers()
          << " miners, weight " << pool->getWeight() << "):" << std::endl
          << "     " << pool->getAlgorithm() << std::endl
          << "     Mined " << pool->getWorkerHashRate() << std::endl
          << "     Submitted " << pool->getSubmitHashRate() << std::endl;
    }
  }

  std::cout << out.str() << std::endl;
}

void Proxy::printMinerStatus()
{
  std::ostringstream out;

  out << std::endl << "Status of the " << clients_.size() << " connected miner(s):" << std::endl;
  for (auto& client : clients_)
  {
    out << " " << client->getIdentifier() << " " << client->getHashRate() << std::endl;
  }

  std::cout << out.str() << std::endl;
}


} // namespace proxy
} // namespace ses