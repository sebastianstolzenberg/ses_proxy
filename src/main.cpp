#include <iostream>
#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>

#include "proxy/proxy.hpp"

//class MainServerHandler : public ses::net::server::ServerHandler,
//                          public ses::net::ConnectionHandler
//{
//public:
//  virtual void handleNewConnection(const ses::net::Connection::Ptr& connection)
//  {
//    std::cout << "New Connection" << std::endl;
//    connection_ = connection;
//    connection_->send("Server Hello");
//  }
//
//  virtual void handleReceived(char* data, size_t size)
//  {
//    std::cout << "handleReceived ";
//    std::cout.write(data, size);
//    std::cout << std::endl;
//  }
//
//  virtual void handleError(const std::string &error)
//  {
//
//  }
//
//  virtual void handleDisconnected()
//  {
//
//  }
//
//private:
//  ses::net::Connection::Ptr connection_;
//};

void waitForSignal()
{
  // waits for signals ending program
  boost::asio::io_service ioService;
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
  ioService.run();
}

int main()
{
//  std::shared_ptr<MainServerHandler> handler = std::make_shared<MainServerHandler>();
//  ses::net::server::Server::Ptr server =
//    ses::net::server::createServer(handler, "127.0.0.1", 55555);
//
//  ses::net::Connection::Ptr clientCon =
//    ses::net::client::establishConnection(handler, "127.0.0.1", 55555);
//
//  clientCon->send("Client Hello");
//
//  sleep(1);

  ses::proxy::Proxy::Ptr proxy = std::make_shared<ses::proxy::Proxy>();



//  proxy->addPool(ses::proxy::Pool::Configuration(ses::net::EndPoint("127.0.0.1", 5555),
//                                                 "WmtUmjUrDQNdqTtau95gJN6YTUd9GWxK4AmgqXeAXLwX8U6eX9zECuALB1Fcwoa8pJJNoniFPo5Kdix8EUuFsUaz1rwKfhCw4",
//                                                 "ses-proxy-test",
//                                                 ses::proxy::ALGORITHM_CRYPTONIGHT));
  proxy->addPool(ses::proxy::Pool::Configuration(ses::net::EndPoint("pool.aeon.hashvault.pro", 443),
                                                 "WmtUmjUrDQNdqTtau95gJN6YTUd9GWxK4AmgqXeAXLwX8U6eX9zECuALB1Fcwoa8pJJNoniFPo5Kdix8EUuFsUaz1rwKfhCw4",
                                                 "ses-proxy-test",
                                                 ses::proxy::ALGORITHM_CRYPTONIGHT));

  proxy->addServer(ses::proxy::Server::Configuration(ses::net::EndPoint("127.0.0.1", 12345),
                                                     ses::proxy::ALGORITHM_CRYPTONIGHT));

//  ses::proxy::Server::Ptr proxyServer = std::make_shared<ses::proxy::Server>();
//  proxyServer->start(ses::proxy::Server::Configuration(ses::net::EndPoint("127.0.0.1", 12345),
//                                                       ses::proxy::ALGORITHM_CRYPTONIGHT),
//                     handleNewClient);
//
//
//  ses::proxy::Pool::Ptr pool = std::make_shared<ses::proxy::Pool>();
//  pool->connect(ses::proxy::Pool::Configuration(ses::net::EndPoint("127.0.0.1", 5555),
//                                                "WmtUmjUrDQNdqTtau95gJN6YTUd9GWxK4AmgqXeAXLwX8U6eX9zECuALB1Fcwoa8pJJNoniFPo5Kdix8EUuFsUaz1rwKfhCw4",
//                                                "ses-proxy-test",
//                                                ses::proxy::ALGORITHM_CRYPTONIGHT));

  waitForSignal();

  return 0;
}