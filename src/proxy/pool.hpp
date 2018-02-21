//
// Created by ses on 18.02.18.
//

#ifndef SES_PROXY_POOL_HPP
#define SES_PROXY_POOL_HPP

#include <memory>
#include <unordered_map>
#include "net/connection.hpp"
#include "stratum/stratum.hpp"

namespace ses {
namespace proxy {

class Pool : public net::ConnectionHandler,
             public std::enable_shared_from_this<Pool>
{
public:
  typedef std::shared_ptr<Pool> Ptr;

public:
  void connect(const std::string& host, uint16_t port, const std::string& user, const std::string& pass,
               net::ConnectionType connectionType = net::CONNECTION_TYPE_AUTO);

  void getJob();

  void submit(const std::string& nonce, const std::string& result);

private: // net::ConnectionHandler
  void handleReceived(char* data, std::size_t size) override;
  void handleError(const std::string& error) override;

public:
  void handleLoginSuccess(const std::string& id, const stratum::Job::Ptr& job);
  void handleLoginError(int code, const std::string& message);

  void handleGetJobSuccess(const stratum::Job::Ptr& job);
  void handleGetJobError(int code, const std::string& message);

  void handleSubmitSuccess(const std::string& status);
  void handleSubmitError(int code, const std::string& message);

  void handleNewJob(const stratum::Job::Ptr& job);

private:
  typedef uint32_t RequestIdentifier;
  enum RequestType
  {
    REQUEST_TYPE_LOGIN,
    REQUEST_TYPE_GETJOB,
    REQUEST_TYPE_SUBMIT
  };

  void sendRequest(RequestType type, const std::string& params = "");

private:
  net::Connection::Ptr connection_;
  RequestIdentifier nextRequestIdentifier_ = 1;
  std::unordered_map<RequestIdentifier, RequestType> outstandingRequests_;

  std::string clientIdentifier_;
  stratum::Job::Ptr currentJob_;
};

} // namespace proxy
} // namespace ses

#endif //SES_PROXY_POOL_HPP
