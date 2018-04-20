#include <sstream>
#include "proxy/ccclient.hpp"

#include "util/log.hpp"

#undef LOG_COMPONENT
#define LOG_COMPONENT proxy

namespace ses {
namespace proxy {

CcClient::Status::Status()
  : hashRateShort_(0), hashRateMedium_(0), hashRateLong_(0), hashRateHighest_(0), currentThreads_(0), sharesGood_(0),
    sharesTotal_(0), numMiners_(0)
{}

std::string CcClient::Status::toJson() const
{
  std::ostringstream json;

  json << "{\"client_status\":{"
       <<  "\"client_id\":\"" << clientId_ << "\","
       <<  "\"current_status\":\"RUNNING\","
       <<  "\"current_pool\":\"" << currentPool_ << " with " << numMiners_ << " miners\","
       <<  "\"current_algo_name\":\"" << currentAlgoName_ << "\","
       <<  "\"cpu_brand\":\"" << cpuBrand_ << "\","
       <<  "\"external_ip\":\"" << externalIp_ << "\","
       <<  "\"version\":\"" << version_ << "\","
       <<  "\"hashrate_short\":" << hashRateShort_ << ","
       <<  "\"hashrate_medium\":" << hashRateMedium_ << ","
       <<  "\"hashrate_long\":" << hashRateLong_ << ","
       <<  "\"hashrate_highest\":" << hashRateHighest_ << ","
       <<  "\"current_threads\":" << currentThreads_ << ","
       <<  "\"shares_good\":" << sharesGood_ << ","
       <<  "\"shares_total\":" << sharesTotal_ << ","
       <<  "\"hashes_total\":" << hashesTotal_ << ","
       <<  "\"uptime\":" << upTime_
       << "}}";

  return json.str();
}

CcClient::CcClient(const std::shared_ptr<boost::asio::io_service>& ioService, const Configuration& configuration)
  : ioService_(ioService), configuration_(configuration)
{
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

void CcClient::publishStatus(const Status& status)
{
  statusMessageQueue_.emplace(status);
}


void CcClient::send()
{
  if (!statusMessageQueue_.empty())
  {
    if (httpClient_ && httpClient_->isConnected())
    {
      while (!statusMessageQueue_.empty())
      {
        sendStatus(statusMessageQueue_.front());
        statusMessageQueue_.pop();
      }
    }
    else
    {
      connect();
    }
  }
}

void CcClient::connect()
{
  disconnect();

  httpClient_ = std::make_shared<net::client::Http>(ioService_, configuration_.endPoint_, configuration_.userAgent_);
  httpClient_->setBearerAuthenticationToken(configuration_.ccToken_);
  WeakPtr weakSelf = shared_from_this();
  httpClient_->connect(
    [weakSelf]()
    {
      if (auto self = weakSelf.lock())
      {
        self->send();
      }
    },
    [weakSelf](const std::string& error)
    {
//      if (auto self = weakSelf.lock())
//      {
//        self->reconnect();
//      }
    });
}

void CcClient::disconnect()
{
  if (httpClient_)
  {
    httpClient_->disconnect();
    httpClient_.reset();
  }
}

void CcClient::sendStatus(const Status& status)
{
  if (httpClient_)
  {
    std::ostringstream url;
    url << "/client/setClientStatus?clientId=" << configuration_.userAgent_ << "-" << status.clientId_;

    std::ostringstream body;
    body << status.toJson();

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
