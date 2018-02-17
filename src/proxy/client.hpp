//
// Created by ses on 15.02.18.
//

#ifndef SES_PROXY_CLIENT_HPP
#define SES_PROXY_CLIENT_HPP

#include <memory>
#include <list>
#include <map>

#include "net/connection.hpp"
#include "net/jsonrpc/jsonrpc.hpp"
#include "stratum/stratum.hpp"
#include "difficulty.hpp"

namespace ses {
namespace proxy {

class MerkleBranches {};

class Client : public net::ConnectionHandler,
               public net::jsonrpc::ParserHandler,
               public stratum::ServerMethodHandler,
               public std::enable_shared_from_this<Client>
{
public:
  typedef std::shared_ptr<Client> Ptr;

public:
  Client();

  void setConnection(const net::Connection::Ptr& connection);

private: // net::ConnectionHandler
  void handleReceived(char* data, std::size_t size) override;
  void handleError(const std::string& error) override;

private: // net::jsonrpc::ParserHandler
  void handleJsonRequest(const std::string& id, const std::string& method, const std::string& params) override;
  void handleJsonResponse(const std::string& id, const std::string& result, const std::string& error) override;
  void handleJsonNotification(const std::string& id, const std::string& method, const std::string& params) override;

private: // stratum::ServerMethodHandler
  void handleStratumServerLogin(const std::string& jsonRequestId,
                                const std::string& login, const std::string& pass, const std::string& agent) override;
  void handleStratumServerGetJob(const std::string& jsonRequestId) override;
  void handleStratumServerSubmit(const std::string& jsonRequestId,
                                 const std::string& identifier, const std::string& jobIdentifier,
                                 const std::string& nonce, const std::string& result) override;
  void handleStratumServerKeepAliveD(const std::string& jsonRequestId) override;
  void handleStratumServerMiningAuthorize(const std::string& username, const std::string& password) override;
  void handleStratumServerMiningCapabilities(const std::string& tbd) override;
  void handleStratumMServeriningExtraNonceSubscribe() override;
  void handleStratumServerMiningGetTransactions(const std::string& jobIdentifier) override;
  void handleStratumServerMiningSubmit(const std::string& username, const std::string& jobIdentifier,
                                       const std::string& extraNonce2, const std::string& nTime,
                                       const std::string& nOnce) override;
  void handleStratumServerMiningSubscribe(const std::string& userAgent, const std::string& extraNonce1) override;
  void handleStratumServerMiningSuggestDifficulty(const std::string& difficulty) override;
  void handleStratumServerMiningSuggestTarget(const std::string& target) override;
    
public:
  void updateVersion();

  void reconnect(std::string hostname, uint16_t port, size_t waittimeS_);

  void notify(std::string jobId,
              std::string hash,
              std::string generationTransactionPart1,
              std::string generationTransactionPart2,
              std::list<MerkleBranches>,
              std::string blockVersion,
              std::string networkDifficulty,
              std::string time,
              bool cleanJobs);

  void setDifficulty(Difficulty difficulty);

  void setExtranone(std::string extranonce1, size_t extranonce2Size);

  void setGoal(std::string name);

private:
  net::Connection::Ptr connection_;

  std::map<std::string, std::string> outstandingRequests_;

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
