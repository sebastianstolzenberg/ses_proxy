#pragma once

#include <memory>
#include <list>
#include <map>
#include <mutex>
#include <functional>
#include <boost/uuid/uuid.hpp>
#include <boost/asio/io_service.hpp>

#include "util/hashratecalculator.hpp"
#include "net/connection.hpp"
#include "proxy/algorithm.hpp"
#include "proxy/job.hpp"
#include "proxy/worker.hpp"

namespace ses {
namespace proxy {

class Client : public Worker,
               public std::enable_shared_from_this<Client>
{
public:
  typedef std::shared_ptr<Client> Ptr;
  typedef std::function<void()> DisconnectHandler;
  typedef std::function<void(const Client::Ptr& client)> NeedsJobHandler;

public:
  Client(const std::shared_ptr<boost::asio::io_service>& ioService,
         const WorkerIdentifier& id, Algorithm defaultAlgorithm,
         uint32_t defaultDifficulty, uint32_t targetSecondsBetweenSubmits);

  void setDisconnectHandler(const DisconnectHandler& disconnectHandler);
  void setConnection(const net::Connection::Ptr& connection);
  void disconnect();
  void setNeedsJobHandler(const NeedsJobHandler& needsJobHandler);

public: // from Worker
  WorkerIdentifier getIdentifier() const override;
  WorkerType getType() const override ;
  bool supports(Algorithm algorithm) const;
  void assignJob(const Job::Ptr& job) override;
  void revokeJob() override;

  bool isConnected() const override;
  std::string getCurrentIp() const override;
  const util::HashRateCalculator& getHashRate() const override;

public:
  bool isLoggedIn() const;
  const std::string& getUseragent() const;
  const std::string& getUsername() const;
  const std::string& getPassword() const;

private:
  void handleReceived(const std::string& data);
  void handleDisconnect(const std::string& error);

  void handleLogin(const std::string& jsonRequestId,
                   const std::string& login, const std::string& pass, const std::string& agent,
                   const std::string& algorithm, const std::vector<std::string>& algorithmVariants);
  void handleGetJob(const std::string& jsonRequestId);
  void handleSubmit(const std::string& jsonRequestId,
                    const std::string& identifier, const std::string& jobIdentifier,
                    const std::string& nonce, const std::string& result,
                    const std::string& workerNonce, const std::string& poolNonce);
  void handleKeepAliveD(const std::string& jsonRequestId, const std::string& identifier);
  void handleUnknownMethod(const std::string& jsonRequestId);

  void handleUpstreamSubmitStatus(std::string jsonRequestId, JobResult::SubmitStatus submitStatus);


private:
  void updateName();
  void updateHashrates(uint32_t difficulty);
  void sendResponse(const std::string& jsonRequestId, const std::string& response);
  void sendSuccessResponse(const std::string& jsonRequestId, const std::string& status);
  void sendErrorResponse(const std::string& jsonRequestId, const std::string& message);
  void sendJobNotification();
  stratum::Job buildStratumJob();

private:
  std::shared_ptr<boost::asio::io_service> ioService_;

  std::recursive_mutex mutex_;

  DisconnectHandler disconnectHandler_;
  NeedsJobHandler needsJobHandler_;

  net::Connection::WeakPtr connection_;
  std::map<std::string, std::string> outstandingRequests_;

  std::string clientName_;
  WorkerIdentifier identifier_;
  AlgorithmType algorithmType_;
  std::set<AlgorithmVariant> algorithmVariants_;
  WorkerType type_;
  bool loggedIn_;
  std::string useragent_;
  std::string username_;
  std::string password_;

  uint32_t difficulty_;
  uint32_t targetSecondsBetweenSubmits_;

  Job::Ptr currentJob_;
  std::map<std::string, std::pair<Job::Ptr, uint32_t> > jobs_;

//  std::chrono::time_point<std::chrono::system_clock> lastJobTransmitTimePoint_;

  size_t submits_;

  util::HashRateCalculator hashrate_;

//  std::string subscribedExtraNone1_;
//  Difficulty suggestedDifficulty_;
//  std::string suggestedTarget_;
};

} // namespace proxy
} // namespace ses
