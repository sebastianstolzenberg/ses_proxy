/* XMRig
 * Copyright 2018      Sebastian Stolzenberg <https://github.com/sebastianstolzenberg>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SES_NET_CLIENT_CONNECTION_H__
#define __SES_NET_CLIENT_CONNECTION_H__

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

#endif /* __SES_NET_CLIENT_CONNECTION_H__ */
