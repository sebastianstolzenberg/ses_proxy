#pragma once

#include <memory>
#include <list>
#include <map>
#include <mutex>
#include <functional>
#include <boost/uuid/uuid.hpp>

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

public:
  Client(const WorkerIdentifier& id, Algorithm defaultAlgorithm);

  void setConnection(const net::Connection::Ptr& connection);

public: // from Worker
  WorkerIdentifier getIdentifier() const override;
  Algorithm getAlgorithm() const override;
  WorkerType getType() const override ;
  void assignJob(const Job::Ptr& job) override;
  bool canHandleJobTemplates() const override;
  void assignJobTemplate(const JobTemplate::Ptr& job) override;

public:
  const std::string& getUseragent() const;
  const std::string& getUsername() const;
  const std::string& getPassword() const;

private:
  void handleReceived(char* data, std::size_t size);
  void handleError(const std::string& error);

  void handleLogin(const std::string& jsonRequestId,
                   const std::string& login, const std::string& pass, const std::string& agent);
  void handleGetJob(const std::string& jsonRequestId);
  void handleSubmit(const std::string& jsonRequestId,
                    const std::string& identifier, const std::string& jobIdentifier,
                    const std::string& nonce, const std::string& result,
                    const std::string& workerNonce, const std::string& poolNonce);
  void handleKeepAliveD(const std::string& jsonRequestId, const std::string& identifier);
  void handleUnknownMethod(const std::string& jsonRequestId);

  void handleUpstreamSubmitStatus(std::string jsonRequestId, JobResult::SubmitStatus submitStatus);


private:
  void sendSuccessResponse(const std::string& jsonRequestId, const std::string& status);
  void sendErrorResponse(const std::string& jsonRequestId, const std::string& message);
  void sendJobNotification();

private:
  std::recursive_mutex mutex_;

  net::Connection::Ptr connection_;
  std::map<std::string, std::string> outstandingRequests_;

  WorkerIdentifier identifier_;
  Algorithm algorithm_;

  WorkerType type_;
  std::string useragent_;
  std::string username_;
  std::string password_;

  Job::Ptr currentJob_;
  std::map<std::string, Job::Ptr> jobs_;

  JobTemplate::Ptr currentJobTemplate_;
  std::map<std::string, JobTemplate::Ptr> jobTemplates_;

//  std::string subscribedExtraNone1_;
//  Difficulty suggestedDifficulty_;
//  std::string suggestedTarget_;
};

} // namespace proxy
} // namespace ses
