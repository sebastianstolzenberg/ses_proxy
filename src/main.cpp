#include <iostream>
#include <memory>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>

#include "proxy/proxy.hpp"

void waitForSignalAndMaxPossibleThreads(boost::asio::io_service& ioService)
{
  // waits for signals ending program

  boost::asio::signal_set signals(ioService);
  signals.add(SIGINT);
  signals.add(SIGTERM);
  signals.add(SIGQUIT);
  signals.async_wait(
    [&](boost::system::error_code /*ec*/, int signo)
    {
      std::cout << "Signal " << signo << " received ... exiting" << std::endl;
      ioService.stop();
    });

  const uint32_t numThreads = std::thread::hardware_concurrency();
  std::cout << std::endl << "Launching " << numThreads << " worker threads." << std::endl;
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

  ses::proxy::Proxy::Ptr proxy = std::make_shared<ses::proxy::Proxy>(ioService);

//  proxy->addPool(ses::proxy::Pool::Configuration(ses::net::EndPoint("127.0.0.1", 5555),
//                                                 "WmtUmjUrDQNdqTtau95gJN6YTUd9GWxK4AmgqXeAXLwX8U6eX9zECuALB1Fcwoa8pJJNoniFPo5Kdix8EUuFsUaz1rwKfhCw4",
//                                                 "ses-proxy-test",
//                                                 ses::proxy::ALGORITHM_CRYPTONIGHT));
  proxy->addPool(ses::proxy::Pool::Configuration(ses::net::EndPoint("pool.aeon.hashvault.pro", 443),
                                                 "WmtUmjUrDQNdqTtau95gJN6YTUd9GWxK4AmgqXeAXLwX8U6eX9zECuALB1Fcwoa8pJJNoniFPo5Kdix8EUuFsUaz1rwKfhCw4",
                                                 "ses-proxy-test",
                                                 ses::proxy::ALGORITHM_CRYPTONIGHT));

  proxy->addServer(ses::proxy::Server::Configuration(ses::net::EndPoint("127.0.0.1", 12345),
                                                     ses::proxy::ALGORITHM_CRYPTONIGHT, 5000));

  waitForSignalAndMaxPossibleThreads(*ioService);



  return 0;
}