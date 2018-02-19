//
// Created by ses on 15.02.18.
//

#ifndef SES_PROXY_CLIENT_HPP
#define SES_PROXY_CLIENT_HPP

#include <memory>
#include <list>
#include <map>
#include <boost/uuid/uuid.hpp>

#include "net/connection.hpp"
#include "difficulty.hpp"

namespace ses {
namespace proxy {

class MerkleBranches {};

class Client : public net::ConnectionHandler,
               public std::enable_shared_from_this<Client>
{
public:
  typedef std::shared_ptr<Client> Ptr;

public:
  Client(const boost::uuids::uuid& id);

  void setConnection(const net::Connection::Ptr& connection);

private: // net::ConnectionHandler
  void handleReceived(char* data, std::size_t size) override;
  void handleError(const std::string& error) override;

public:
  void handleLogin(const std::string& jsonRequestId,
                   const std::string& login, const std::string& pass, const std::string& agent);
  void handleGetJob(const std::string& jsonRequestId);
  void handleSubmit(const std::string& jsonRequestId,
                    const std::string& identifier, const std::string& jobIdentifier,
                    const std::string& nonce, const std::string& result);
  void handleKeepAliveD(const std::string& jsonRequestId, const std::string& identifier);
  void handleUnknownMethod(const std::string& jsonRequestId);

private:
  void sendSuccessResponse(const std::string& jsonRequestId, const std::string& status);
  void sendErrorResponse(const std::string& jsonRequestId, const std::string& message);

private:
  net::Connection::Ptr connection_;
  std::map<std::string, std::string> outstandingRequests_;

  boost::uuids::uuid rpcIdentifier_;

  std::string useragent_;
  std::string username_;
  std::string password_;

  std::string subscribedExtraNone1_;
  Difficulty suggestedDifficulty_;
  std::string suggestedTarget_;
};

} // namespace proxy
} // namespace ses

#endif //SES_PROXY_CLIENT_HPP
