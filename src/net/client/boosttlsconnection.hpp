#pragma once

#include "net/client/connection.hpp"

namespace ses {
namespace net {
namespace client {

Connection::Ptr establishBoostTlsConnection(const std::shared_ptr<boost::asio::io_service>& ioService,
                                            const std::string& host, uint16_t port,
                                            const Connection::ReceivedDataHandler& receivedDataHandler,
                                            const Connection::ErrorHandler& errorHandler);

} //namespace client
} //namespace ses
} //namespace net
