//
// Created by ses on 15.02.18.
//

#ifndef SES_PROXY_CLIENT_HPP
#define SES_PROXY_CLIENT_HPP

#include <memory>
#include <list>

#include "../difficulty.hpp"

namespace ses {
namespace proxy {

class MerkleBranches {};

class Client
{
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
