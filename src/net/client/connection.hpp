#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>

#include "net/connection.hpp"
#include "net/endpoint.hpp"

namespace ses {
namespace net {
namespace client {

Connection::Ptr establishConnection(const EndPoint& endPoint,
                                    const Connection::ReceivedDataHandler& receivedDataHandler,
                                    const Connection::ErrorHandler& errorHandler);

} //namespace client
} //namespace net
} //namespace ses
