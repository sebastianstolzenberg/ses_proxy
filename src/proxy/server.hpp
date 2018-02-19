//
// Created by ses on 17.02.18.
//

#ifndef SES_PROXY_SERVER_HPP
#define SES_PROXY_SERVER_HPP

#include <list>
#include <memory>
#include <boost/uuid/uuid.hpp>

#include "net/server/server.hpp"
#include "proxy/client.hpp"

namespace ses {
namespace proxy {

class Server : public ses::net::server::ServerHandler,
               public std::enable_shared_from_this<ses::proxy::Server>
{
public:
  typedef std::shared_ptr<ses::proxy::Server> Ptr;

public:
  void start(const std::string& address,
             uint16_t port,
             net::ConnectionType type = net::CONNECTION_TYPE_AUTO);

public:
  void handleNewConnection(const net::Connection::Ptr& connection) override;

private:
  net::server::Server::Ptr server_;

  std::map<boost::uuids::uuid, Client::Ptr> clients_;
};

} // namespace proxy
} // namespace ses

#endif //SES_PROXY_SERVER_HPP
