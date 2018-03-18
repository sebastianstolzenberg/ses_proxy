#include <iostream>
#include <memory>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>

#include "proxy/proxy.hpp"
#include "proxy/configurationfile.hpp"
#include "util/log.hpp"

void configureLogging(uint32_t logLevel)
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
      case 2: level = boost::log::trivial::severity_level::error; break;
      case 3: level = boost::log::trivial::severity_level::warning; break;
      case 4: level = boost::log::trivial::severity_level::info; break;
      case 5: level = boost::log::trivial::severity_level::debug; break;
      case 6: level = boost::log::trivial::severity_level::trace; break;
      case 1: // fall through
      default:
        level = boost::log::trivial::severity_level::fatal; break;
    }
    ses::log::setMinimumLogLevel(level);
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
  // this run call uses the main thread
  ioService.run();
}

int main()
{
  std::shared_ptr<boost::asio::io_service> ioService = std::make_shared<boost::asio::io_service>();

  ses::proxy::Configuration configuration = ses::proxy::parseConfigurationFile("config.json");

  configureLogging(configuration.logLevel_);

  ses::proxy::Proxy::Ptr proxy = std::make_shared<ses::proxy::Proxy>(ioService);

  for (const auto& poolConfig : configuration.pools_)
  {
    if (poolConfig.weight_ != 0)
    {
      proxy->addPool(poolConfig);
    }
  }

  for (const auto& serverConfig : configuration.server_)
  {
    proxy->addServer(serverConfig);
  }

  waitForSignal(*ioService, configuration.threads_);

  return 0;
}