#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <list>
#include <mutex>
#include <boost/asio/deadline_timer.hpp>

#include "net/connection.hpp"
#include "net/endpoint.hpp"
#include "stratum/stratum.hpp"
#include "proxy/algorithm.hpp"
#include "proxy/jobtemplate.hpp"
#include "proxy/worker.hpp"

namespace ses {
namespace proxy {

class Pool : public std::enable_shared_from_this<Pool>
{
public:
  typedef std::shared_ptr<Pool> Ptr;

  struct Configuration
  {
    Configuration() : algorithm_(ALGORITHM_CRYPTONIGHT), weight_(0) {}
    Configuration(const net::EndPoint& endPoint, const std::string& user, const std::string& pass,
                  Algorithm algorithm, double weight)
      : endPoint_(endPoint), user_(user), pass_(pass), algorithm_(algorithm), weight_(weight)
    {
    }

    friend std::ostream& operator<<(std::ostream& stream, const Configuration& configuration)
    {
      stream << configuration.endPoint_
             << ", user, " << configuration.user_
             << ", pass, " << configuration.pass_
             << ", algorithm, " << configuration.algorithm_
             << ", weight, " << configuration.weight_;
      return stream;
    }

    net::EndPoint endPoint_;
    std::string user_;
    std::string pass_;
    Algorithm algorithm_;
    double weight_;
  };

public:
  Pool(const std::shared_ptr<boost::asio::io_service>& ioService);
  ~Pool();
  void connect(const Configuration& configuration);

  bool addWorker(const Worker::Ptr& worker);
  bool removeWorker(const Worker::Ptr& worker);
  const std::list<Worker::Ptr>& getWorkersSortedByHashrateDescending();

  const std::string& getDescriptor() const;
  Algorithm getAlgotrithm() const;
  uint32_t getWeight() const;

  size_t numWorkers() const;
  double weightedWorkers() const;

  uint32_t hashRate() const;
  double weightedHashRate() const;

private:
  void handleConnect();
  void handleReceived(const std::string& data);
  void handleDisconnect(const std::string& error);

  void handleLoginSuccess(const std::string& id, const boost::optional<stratum::Job>& job);
  void handleLoginError(int code, const std::string& message);

  void handleGetJobSuccess(const stratum::Job& job);
  void handleGetJobError(int code, const std::string& message);

  void handleSubmitSuccess(const std::string& jobId, const JobResult::SubmitStatusHandler& submitStatusHandler,
                           const std::string& status);
  void handleSubmitError(const std::string& jobId, const JobResult::SubmitStatusHandler& submitStatusHandler,
                         int code, const std::string& message);

  void handleNewJob(const stratum::Job& job);

  JobResult::SubmitStatus handleJobResult(const std::string& workerIdentifier,
                                          const JobResult& jobResult,
                                          const JobResult::SubmitStatusHandler& submitStatusHandler);

private:

  typedef uint32_t RequestIdentifier;
  enum RequestType
  {
    REQUEST_TYPE_LOGIN,
    REQUEST_TYPE_GETJOB,
    REQUEST_TYPE_SUBMIT,
    REQUEST_TYPE_KEEPALIVE
  };
  void updateName();
  RequestIdentifier sendRequest(RequestType type, const std::string& params = "");
  void setJob(const stratum::Job& job);
  void activateJob(const JobTemplate::Ptr& job);
  void removeJob(const std::string& jobId);
  bool assignJobToWorker(const Worker::Ptr& worker);
  void login();
  void submit(const JobResult& jobResult, const JobResult::SubmitStatusHandler& submitStatusHandler);
  void triggerKeepaliveTimer();

private:
  std::shared_ptr<boost::asio::io_service> ioService_;
  std::recursive_mutex mutex_;

  Configuration configuration_;

  net::Connection::Ptr connection_;
  boost::asio::deadline_timer keepaliveTimer_;
  RequestIdentifier nextRequestIdentifier_ = 1;
  std::unordered_map<RequestIdentifier, RequestType> outstandingRequests_;
  std::unordered_map<RequestIdentifier,
    std::tuple<std::string, JobResult::SubmitStatusHandler> > outstandingSubmits_;

  std::string poolName_;
  std::string workerIdentifier_;
  JobTemplate::Ptr activeJobTemplate_;
  std::map<std::string, JobTemplate::Ptr> jobTemplates_;

  std::list<Worker::Ptr> workers_;
};

} // namespace proxy
} // namespace ses
