#include <iostream>
#include <memory>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>

#include "proxy/proxy.hpp"
#include "util/log.hpp"

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
      LOG_WARN << "Signal " << signo << " received ... exiting";
      ioService.stop();
    });

  const uint32_t numThreads = std::thread::hardware_concurrency();
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
  //ses::log::setMinimumLogLevel(boost::log::trivial::severity_level::info);

  std::shared_ptr<boost::asio::io_service> ioService = std::make_shared<boost::asio::io_service>();

  ses::proxy::Proxy::Ptr proxy = std::make_shared<ses::proxy::Proxy>(ioService);

//  proxy->addPool(ses::proxy::Pool::Configuration(ses::net::EndPoint("127.0.0.1", 5555),
//                                                 "WmtUmjUrDQNdqTtau95gJN6YTUd9GWxK4AmgqXeAXLwX8U6eX9zECuALB1Fcwoa8pJJNoniFPo5Kdix8EUuFsUaz1rwKfhCw4",
//                                                 "ses-proxy-test",
//                                                 ses::proxy::ALGORITHM_CRYPTONIGHT));
  proxy->addPool(ses::proxy::Pool::Configuration(ses::net::EndPoint("pool.aeon.hashvault.pro", 443),
                                                 "WmtUmjUrDQNdqTtau95gJN6YTUd9GWxK4AmgqXeAXLwX8U6eX9zECuALB1Fcwoa8pJJNoniFPo5Kdix8EUuFsUaz1rwKfhCw4",
                                                 "ses-proxy-test",
                                                 ses::proxy::ALGORITHM_CRYPTONIGHT_LITE,
                                                 10));

  proxy->addPool(ses::proxy::Pool::Configuration(ses::net::EndPoint("pool.monero.hashvault.pro", 443, ses::net::CONNECTION_TYPE_TCP),
                                                 "48fAeJSo6yTitJ8KVmVFzAB5Pj7Vf5hEybRupdGejGnfK93yJccnaQ6jidiQqtj1LTWy1J95HuFAMg46eUKLahEKLBFpn85",
                                                 "ses-proxy-test",
                                                 ses::proxy::ALGORITHM_CRYPTONIGHT,
                                                 50));

  proxy->addPool(ses::proxy::Pool::Configuration(ses::net::EndPoint("pool.intense.hashvault.pro", 5555),
                                                 "iz5w2C5AjmnM8ArCcBRryETYEMYnJWGVmKMDFbP5HkzLN3oZsf3grtwGykdzCVj23Thco232PGrwHL6uQProtvsa28bojsNFG",
                                                 "ses-proxy-test",
                                                 ses::proxy::ALGORITHM_CRYPTONIGHT,
                                                 10));

  proxy->addServer(ses::proxy::Server::Configuration(ses::net::EndPoint("127.0.0.1", 12345),
                                                     ses::proxy::ALGORITHM_CRYPTONIGHT, 5000));
  proxy->addServer(ses::proxy::Server::Configuration(ses::net::EndPoint("127.0.0.1", 12346),
                                                     ses::proxy::ALGORITHM_CRYPTONIGHT_LITE, 5000));

  waitForSignalAndMaxPossibleThreads(*ioService);



  return 0;
}