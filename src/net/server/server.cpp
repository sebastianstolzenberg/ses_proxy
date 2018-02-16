//
// Created by ses on 16.02.18.
//

#include "net/server/server.hpp"

namespace ses {
namespace net {
namespace server {

Server::Ptr createServer(const ServerHandler::Ptr& handler,
                         ConnectionType type,
                         const std::string& address,
                         uint16_t port)
{

}

} //namespace server
} //namespace net
} //namespace ses