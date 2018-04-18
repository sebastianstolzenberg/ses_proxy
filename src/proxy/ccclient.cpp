#include <sstream>
#include "proxy/ccclient.hpp"

#include "util/log.hpp"

#undef LOG_COMPONENT
#define LOG_COMPONENT proxy

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
        self->publishStatus();
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
    std::ostringstream url;
    url << "/client/setClientConfig?clientId=" << configuration_.userAgent_;

    std::ostringstream body;
    body << "{\"test\":true}";

    httpClient_->post(url.str(), "application/json", body.str(),
                      [](const std::string& response){
                        LOG_INFO << "publishConfig() success - " << response;
                      },
                      [](const std::string& error)
                      {
                        LOG_ERROR << "publishConfig() error - " << error;
                      });
  }
}

void CcClient::publishStatus()
{
  if (httpClient_)
  {
    std::ostringstream url;
    url << "/client/setClientStatus?clientId=" << configuration_.userAgent_;

    std::ostringstream body;
    body << "{\"client_status\":{\"client_id\":\"ses-proxy\"}}";

    httpClient_->post(url.str(), "application/json", body.str(),
                      [](const std::string& response){
                        LOG_INFO << "publishStatus() success - " << response;
                      },
                      [](const std::string& error)
                      {
                        LOG_ERROR << "publishStatus() error - " << error;
                      });
  }
}

} // namespace proxy
} // namespace ses
