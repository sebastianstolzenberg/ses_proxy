#pragma once

#include <string>
#include "connectiontype.hpp"

namespace ses {
namespace net {

class EndPoint
{
public:
  EndPoint() : connectionType_(CONNECTION_TYPE_AUTO) {}
  EndPoint(const std::string& host, uint16_t port, ConnectionType connectionType = CONNECTION_TYPE_AUTO)
    : host_(host), port_(port), connectionType_(connectionType)
  {
  }

  friend std::ostream& operator<<(std::ostream& stream, const EndPoint& endPoint)
  {
    stream << "host, " << endPoint.host_
           << ", port, " << endPoint.port_
           << ", type, " << endPoint.connectionType_;
    return stream;
  }

  std::string host_;
  uint16_t port_;
  ConnectionType connectionType_;

  std::string certificateChainFile_;
  std::string privateKeyFile_;
};
} //namespace net
} //namespace ses
