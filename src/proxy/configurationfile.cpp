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

void parsePoolConfigurations(boost::property_tree::ptree& ptree, std::list<Pool::Configuration>& list)
{
  double totalWeight = 0;
  for (auto& pool : ptree.get_child("pools"))
  {
    Pool::Configuration configuration;
    configuration.endPoint_.host_ = pool.second.get<std::string>("host");
    configuration.endPoint_.port_ = pool.second.get<uint16_t>("port");
    configuration.endPoint_.connectionType_ =
      parseConnectionType(pool.second.get<std::string>("connectionType", "auto"));
    configuration.user_ = pool.second.get<std::string>("username");
    configuration.pass_ = pool.second.get<std::string>("password");
    configuration.weight_ = std::fmax(0.0, pool.second.get<double>("weight"));
    totalWeight += configuration.weight_;
    configuration.algorithm_ = toAlgorithm(pool.second.get<std::string>("algorithm", ""));
    list.push_back(configuration);
  }

  if (totalWeight > 0.0)
  {
    for (auto& configuration : list)
    {
      configuration.weight_ /= totalWeight;
    }
  }
}

void parseServerConfigurations(boost::property_tree::ptree& ptree, std::list<Server::Configuration>& list)
{
  for (auto& server : ptree.get_child("server"))
  {
    Server::Configuration configuration;
    configuration.endPoint_.host_ = server.second.get<std::string>("host");
    configuration.endPoint_.port_ = server.second.get<uint16_t>("port");
    configuration.endPoint_.connectionType_ =
      parseConnectionType(server.second.get<std::string>("connectionType", "auto"));
    configuration.endPoint_.certificateChainFile_ = server.second.get<std::string>("certificateChainFile", "");
    configuration.endPoint_.privateKeyFile_ = server.second.get<std::string>("privateKeyFile", "");
    configuration.defaultAlgorithm_ = toAlgorithm(server.second.get<std::string>("defaultAlgorithm", ""));
    configuration.defaultAlgorithmVariant_ = toAlgorithmVariant(server.second.get<std::string>("defaultAlgorithmVariant", ""));
    configuration.defaultDifficulty_ = server.second.get<uint32_t>("defaultDifficulty", 5000);
    configuration.targetSecondsBetweenSubmits_ = server.second.get<uint32_t>("targetSecondsBetweenSubmits", 15);
    list.push_back(configuration);
  }
}

void parseCcClientConfigurations(boost::property_tree::ptree& ptree,
                                 boost::optional<CcClient::Configuration>& ccClientConfiguration)
{
  if (ptree.count("ccClient") > 0)
  {
    auto& ccClient = ptree.get_child("ccClient");
    CcClient::Configuration configuration;
    configuration.endPoint_.host_ = ccClient.get<std::string>("host");
    configuration.endPoint_.port_ = ccClient.get<uint16_t>("port");
    configuration.endPoint_.connectionType_ =
      parseConnectionType(ccClient.get<std::string>("connectionType", "auto"));
    configuration.ccToken_ = ccClient.get<std::string>("accessToken", "");
    configuration.userAgent_ = ccClient.get<std::string>("workerId", "");
    configuration.ccToken_ = ccClient.get<std::string>("accessToken", "");
    configuration.updateIntervalSeconds_ = ccClient.get<uint32_t>("updateIntervalSeconds", 10);
    ccClientConfiguration = configuration;
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
  parseCcClientConfigurations(ptree, configuration.ccCient_);
  configuration.logLevel_ = ptree.get<uint32_t>("logLevel", 4);
  configuration.threads_ = ptree.get<size_t>("threads", 0);
  configuration.poolLoadBalanceIntervalSeconds_ = ptree.get<uint32_t>("poolLoadBalanceIntervalSeconds", 20);
  return configuration;
}

} // namespace proxy
} // namespace ses
