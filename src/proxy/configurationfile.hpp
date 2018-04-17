#pragma once

#include <list>

#include "proxy/pool.hpp"
#include "proxy/server.hpp"
#include "proxy/ccclient.hpp"

namespace ses {
namespace proxy {

struct Configuration
{
  std::list<Pool::Configuration> pools_;
  std::list<Server::Configuration> server_;
  boost::optional<CcClient::Configuration> ccCient_;
  uint32_t logLevel_;
  size_t threads_;
  uint32_t poolLoadBalanceIntervalSeconds_;
};

Configuration parseConfigurationFile(const std::string& fileName);

} // namespace proxy
} // namespace ses
