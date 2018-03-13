#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "proxy/configurationfile.hpp"

namespace ses {
namespace proxy {

namespace {
net::ConnectionType parseConnectionType(const std::string& connectionTypeString)
{
  net::ConnectionType connectionType = net::ConnectionType::CONNECTION_TYPE_AUTO;
  std::string compare = connectionTypeString;
  boost::algorithm::to_lower(compare);
  if (compare == "tls")
  {
    connectionType = net::ConnectionType::CONNECTION_TYPE_TLS;
  }
  else if (compare == "tcp")
  {
    connectionType = net::ConnectionType::CONNECTION_TYPE_TCP;
  }
  return connectionType;
}

Algorithm parseAlgorithm(const std::string& algorithmString)
{
  Algorithm algorithm = Algorithm::ALGORITHM_CRYPTONIGHT;
  std::string compare = algorithmString;
  boost::algorithm::to_lower(compare);
  if (compare == "cryptonight-lite")
  {
    algorithm = Algorithm::ALGORITHM_CRYPTONIGHT_LITE;
  }
  return algorithm;
}

void parsePoolConfigurations(boost::property_tree::ptree& ptree, std::list<Pool::Configuration>& list)
{
  for (auto& pool : ptree.get_child("pools"))
  {
    Pool::Configuration configuration;
    configuration.endPoint_.host_ = pool.second.get<std::string>("host");
    configuration.endPoint_.port_ = pool.second.get<uint16_t>("port");
    configuration.endPoint_.connectionType_ =
      parseConnectionType(pool.second.get<std::string>("connectionType", "auto"));
    configuration.user_ = pool.second.get<std::string>("username");
    configuration.pass_ = pool.second.get<std::string>("password");
    configuration.weight_ = pool.second.get<uint32_t>("weight");
    configuration.algorithm_ = parseAlgorithm(pool.second.get<std::string>("algorithm", ""));
    list.push_back(configuration);
  }
}

void parseServerConfigurations(boost::property_tree::ptree& ptree, std::list<Server::Configuration>& list)
{
  for (auto& pool : ptree.get_child("server"))
  {
    Server::Configuration configuration;
    configuration.endPoint_.host_ = pool.second.get<std::string>("host");
    configuration.endPoint_.port_ = pool.second.get<uint16_t>("port");
    configuration.endPoint_.connectionType_ =
      parseConnectionType(pool.second.get<std::string>("connectionType", "auto"));
    configuration.defaultAlgorithm_ = parseAlgorithm(pool.second.get<std::string>("defaultAlgorithm", ""));
    configuration.defaultDifficulty_ = pool.second.get<uint32_t>("defaultDifficulty", 0);
    list.push_back(configuration);
  }
}
}

//TODO test malformed config files

Configuration parseConfigurationFile(const std::string& fileName)
{
  boost::property_tree::ptree ptree;
  boost::property_tree::json_parser::read_json(fileName, ptree);

  Configuration configuration;
  parsePoolConfigurations(ptree, configuration.pools_);
  parseServerConfigurations(ptree, configuration.server_);
  return configuration;
}

} // namespace proxy
} // namespace ses
