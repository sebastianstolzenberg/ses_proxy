#include "proxy/ccclient.hpp"

namespace ses {
namespace proxy {

CcClient::CcClient(const std::shared_ptr<boost::asio::io_service>& ioService)
  : ioService_(ioService)
{

}

void CcClient::connect(const Configuration& configuration)
{
  configuration_ = configuration;

  httpClient_ = std::make_shared<net::client::Http>(ioService_, configuration_.endPoint_, configuration_.userAgent_);
  httpClient_->setBearerAuthenticationToken(configuration_.ccToken_);
  WeakPtr weakSelf = shared_from_this();
  httpClient_->connect(
    [weakSelf]()
    {
      if (auto self = weakSelf.lock())
      {
        self->publishConfig();
      }
    },
    [weakSelf](const std::string& error)
    {
      if (auto self = weakSelf.lock())
      {
        self->httpClient_.reset();
      }
    });
}

void CcClient::publishConfig()
{
  if (httpClient_)
  {
    httpClient_->post("/client/setClientConfig?clientId=", "body",
                      [](const std::string& response){},
                      [](const std::string& error){});
  }
}

} // namespace proxy
} // namespace ses
