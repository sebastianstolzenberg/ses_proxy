#ifndef SES_NET_ENDPOINT_HPP
#define SES_NET_ENDPOINT_HPP

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

  std::string host_;
  uint16_t port_;
  ConnectionType connectionType_;
};
} //namespace net
} //namespace ses

#endif //SES_NET_ENDPOINT_HPP
