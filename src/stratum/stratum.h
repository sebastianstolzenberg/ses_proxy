//
// Created by ses on 15.02.18.
//

#ifndef SES_PROXY_STRATUM_H
#define SES_PROXY_STRATUM_H

namespace ses {
namespace stratum {

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

#endif //SES_PROXY_STRATUM_H
