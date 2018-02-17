
#include "proxy/server.hpp"

namespace ses {
namespace proxy {

void Server::start(const std::string& address, uint16_t port, net::ConnectionType type)
{
  Server::Ptr server = shared_from_this();
  server_ = net::server::createServer(server, address, port, type);
}

void Server::handleNewConnection(const net::Connection::Ptr& connection)
{
  Client::Ptr client = std::make_shared<Client>();
  client->setConnection(connection);
  clients_.push_back(client);
}

} // namespace proxy
} // namespace ses