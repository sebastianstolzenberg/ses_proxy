#pragma once

#include <list>
#include <memory>
#include <functional>
#include <boost/uuid/uuid.hpp>
#include <boost/asio/io_service.hpp>

#include "net/server/server.hpp"
#include "proxy/client.hpp"
#include "proxy/algorithm.hpp"

namespace ses {
namespace proxy {

class Server : public std::enable_shared_from_this<ses::proxy::Server>
{
public:
  typedef std::shared_ptr<ses::proxy::Server> Ptr;

  typedef std::function<void(Client::Ptr newClient)> NewClientHandler;

  struct Configuration
  {
    Configuration() : defaultAlgorithm_(ALGORITHM_CRYPTONIGHT), defaultDifficulty_(5000),
      targetSecondsBetweenSubmits_(15) {}
    Configuration(const net::EndPoint& endPoint, Algorithm defaultAlgorithm, uint32_t defaultDifficulty,
                  uint32_t targetSecondsBetweenSubmits)
      : endPoint_(endPoint), defaultAlgorithm_(defaultAlgorithm), defaultDifficulty_(defaultDifficulty),
        targetSecondsBetweenSubmits_(targetSecondsBetweenSubmits)
    {
    }

    friend std::ostream& operator<<(std::ostream& stream, const Configuration& configuration)
    {
      stream << configuration.endPoint_
             << ", defaultAlgorithm, " << configuration.defaultAlgorithm_
             << ", defaultDifficulty, " << configuration.defaultDifficulty_
             << ", targetSecondsBetweenSubmits, " << configuration.targetSecondsBetweenSubmits_;
      return stream;
    }

    net::EndPoint endPoint_;
    Algorithm defaultAlgorithm_;
    uint32_t defaultDifficulty_;
    uint32_t targetSecondsBetweenSubmits_;
  };

public:
  Server(const std::shared_ptr<boost::asio::io_service>& ioService);
  void start(const Configuration& configuration, const NewClientHandler& newClientHandler);

public:
  void handleNewConnection(net::Connection::Ptr connection);

private:
  std::shared_ptr<boost::asio::io_service> ioService_;
  Configuration configuration_;
  NewClientHandler newClientHandler_;
  net::server::Server::Ptr server_;
};

} // namespace proxy
} // namespace ses
