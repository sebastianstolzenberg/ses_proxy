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
  };

public:
  CcClient(const std::shared_ptr<boost::asio::io_service>& ioService);
  void connect(const Configuration& configuration);

  void publishConfig();

private:
  std::shared_ptr<boost::asio::io_service> ioService_;
  Configuration configuration_;
  net::client::Http::Ptr httpClient_;
};

} // namespace proxy
} // namespace ses
