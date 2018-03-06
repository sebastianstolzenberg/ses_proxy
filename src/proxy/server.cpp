#include <boost/uuid/random_generator.hpp>

#include "proxy/server.hpp"

namespace ses {
namespace proxy {

Server::Server(const std::shared_ptr<boost::asio::io_service>& ioService)
  : ioService_(ioService)
{
}

void Server::start(const Configuration& configuration, const NewClientHandler& newClientHandler)
{
  configuration_ = configuration;
  newClientHandler_ = newClientHandler;
  server_ = net::server::createServer(ioService_,
                                      std::bind(&Server::handleNewConnection, this,
                                                std::placeholders::_1),
                                      configuration.endPoint_);
}

void Server::handleNewConnection(net::Connection::Ptr connection)
{
  auto workerId = boost::uuids::random_generator()();
  auto client = std::make_shared<Client>(ioService_, workerId, configuration_.defaultAlgorithm_);
  client->setConnection(connection);
  if (newClientHandler_)
  {
    newClientHandler_(client);
  }
}

} // namespace proxy
} // namespace ses