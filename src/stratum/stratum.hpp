#ifndef SES_STRATUM_STRATUM_HPP
#define SES_STRATUM_STRATUM_HPP

#include <string>

namespace ses {
namespace stratum {

class ServerMethodHandler
{
protected:
  virtual ~ServerMethodHandler() {}

public:
  virtual void handleStratumServerLogin(const std::string& jsonRequestId, const std::string& login, const std::string& pass, const std::string& agent) = 0;
  virtual void handleStratumServerGetJob(const std::string& jsonRequestId) = 0;
  virtual void handleStratumServerSubmit(const std::string& jsonRequestId,
                                         const std::string& identifier, const std::string& jobIdentifier,
                                         const std::string& nonce, const std::string& result) = 0;
  virtual void handleStratumServerKeepAliveD(const std::string& jsonRequestId) = 0;

  virtual void handleStratumServerMiningAuthorize(const std::string& username, const std::string& password) = 0;
  virtual void handleStratumServerMiningCapabilities(const std::string& tbd) = 0;
  virtual void handleStratumMServeriningExtraNonceSubscribe() = 0;
  virtual void handleStratumServerMiningGetTransactions(const std::string& jobIdentifier) = 0;
  virtual void handleStratumServerMiningSubmit(const std::string& username, const std::string& jobIdentifier,
                                         const std::string& extraNonce2, const std::string& nTime,
                                         const std::string& nOnce) = 0;
  virtual void handleStratumServerMiningSubscribe(const std::string& userAgent, const std::string& extraNonce1) = 0;
  virtual void handleStratumServerMiningSuggestDifficulty(const std::string& difficulty) = 0;
  virtual void handleStratumServerMiningSuggestTarget(const std::string& target) = 0;
};

class ClientMethodHandler
{
protected:
  virtual ~ClientMethodHandler() {}

public:
  virtual void handleStratumClientGetVersion() = 0;
  virtual void handleStratumClientReconnect(const std::string& host, const std::string& port, const std::string& waittime) = 0;
  virtual void handleStratumClientShowMessage(const std::string& message) = 0;
  virtual void handleStratumClientMiningNotify(const std::string& jobIdentifier, const std::string& previousHash,
                                               const std::string& conb1, const std::string& coinb2,
                                               const std::string& merkleBranch, const std::string& version,
                                               const std::string& nBits, const std::string& nTime, bool cleanJobs) = 0;
  virtual void handleStratumClientMiningSetDifficulty(const std::string& difficulty) = 0;
  virtual void handleStratumClientMiningSetExtraNonce(const std::string& extraNonce1, size_t extraNonce2Size) = 0;
  virtual void handleStratumClientMiningSetGoal(const std::string& tbd) = 0;
};

void parseServerMethod(const std::string& jsonRequestId, ServerMethodHandler& handler,
                       const std::string& method, const std::string& params);

void parseClientMethod(const std::string& jsonRequestId, ClientMethodHandler& handler,
                       const std::string& method, const std::string& params);

class Job
{
public:
  std::string blob_;
  std::string jobId_;
  std::string target_;
  std::string id_;
};

std::string createLoginResponse(const std::string& id, const Job& job, const std::string& status);
std::string createJobNotification(const Job& job);
std::string createSubmitResponse(bool ok);

namespace client {

struct getVersion
{
};

struct reconnect
{
  std::string hostname_;
  std::string port_;
  std::string waittime_;
};

struct showMessage
{
  std::string message_;
};

} // namespace client

namespace mining {

struct authorize
{
  std::string username_;
  std::string password_;
};

struct capabilities
{
  std::string notify_;
  std::string setDifficulty_;
  std::string setGoal_;
  std::string suggestedTarget_;
  std::string hexTarget_;
};

struct extranonceSubscribe
{
};

struct getTransactions
{
  std::string jobId_;
};

struct submit
{
  std::string username_;
  std::string jobId_;
  std::string extraNonce2;
  std::string nTime_;
  std::string nOnce_;
};

struct subscribe
{
  std::string userAgent_;
  std::string extraNonce1_;
};

struct suggestDifficulty
{
  std::string difficulty_;
};

struct suggestTarget
{
  std::string target_;
};

struct setDifficulty
{
  std::string difficulty_;
};

struct setExtranonce
{
  std::string extranonce1_;
  std::string exranonce2Size_;
};

struct setGoal
{
  std::string name;

};

} // namespace mining

} // namespace stratum
} // namespace ses

#endif //SES_STRATUM_STRATUM_HPP
