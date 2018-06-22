#pragma once

namespace ses {
namespace net {

//TODO http proxy traversal

enum ConnectionType
{
  CONNECTION_TYPE_AUTO,
  CONNECTION_TYPE_TCP,
  CONNECTION_TYPE_TLS
};

inline std::string toString(ConnectionType connectionType)
{
  switch (connectionType)
  {
    case ConnectionType::CONNECTION_TYPE_AUTO:
      return "auto";
    case ConnectionType::CONNECTION_TYPE_TCP:
      return "tcp";
    case ConnectionType::CONNECTION_TYPE_TLS:
      return "tls";
    default:
      return "unknown";
  }
}

} //namespace net
} //namespace ses
