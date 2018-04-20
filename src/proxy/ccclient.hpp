#pragma once

#include <memory>
#include <boost/asio/io_service.hpp>

#include "net/client/http.hpp"

namespace ses {
namespace proxy {

class CcClient : public std::enable_shared_from_this<CcClient>
{
public:
  typedef std::shared_ptr<CcClient> Ptr;
  typedef std::weak_ptr<CcClient> WeakPtr;

  struct Status
  {
    std::string clientId_;
    std::string status_;

    std::string currentPool_;
    std::string currentAlgoName_;
    std::string cpuBrand_;
    std::string externalIp_;
    std::string version_;

    double hashRateShort_;
    double hashRateMedium_;
    double hashRateLong_;
    double hashRateHighest_;

    size_t currentThreads_;
    size_t sharesGood_;
    size_t sharesTotal_;
    size_t hashesTotal_;

    size_t numMiners_;

    Status();
    std::string toJson() const;
  };

  struct Configuration
  {
    Configuration() {}
    Configuration(const net::EndPoint& endPoint, const std::string& userAgent, const std::string& ccToken)
      : endPoint_(endPoint), userAgent_(userAgent), ccToken_(ccToken)
    {
    }

    net::EndPoint endPoint_;
    std::string userAgent_;
    std::string ccToken_;
    uint32_t updateInteralSeconds_;
  };

public:
  CcClient(const std::shared_ptr<boost::asio::io_service>& ioService);
  void connect(const Configuration& configuration);
  void reconnect();

  void publishConfig();
  void publishStatus(const Status& status);

private:
  std::shared_ptr<boost::asio::io_service> ioService_;
  Configuration configuration_;
  net::client::Http::Ptr httpClient_;
};

} // namespace proxy
} // namespace ses
