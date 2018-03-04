#pragma once

#include "net/client/connection.hpp"

namespace ses {
namespace net {
namespace client {

Connection::Ptr establishBoostTcpConnection(const std::string& host, uint16_t port,
                                            const Connection::ReceivedDataHandler& receivedDataHandler,
                                            const Connection::ErrorHandler& errorHandler);

} //namespace client
} //namespace ses
} //namespace net
