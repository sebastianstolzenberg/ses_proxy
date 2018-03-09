#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/asio/io_service.hpp>

#include "net/connection.hpp"
#include "net/endpoint.hpp"

namespace ses {
namespace net {
namespace client {

Connection::Ptr establishConnection(const std::shared_ptr<boost::asio::io_service>& ioService,
                                    const EndPoint& endPoint,
                                    const Connection::ReceivedDataHandler& receivedDataHandler,
                                    const Connection::DisconnectHandler& errorHandler);

} //namespace client
} //namespace net
} //namespace ses
