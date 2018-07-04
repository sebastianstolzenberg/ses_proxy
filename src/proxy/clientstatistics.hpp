#pragma once

#include <string>
#include <ostream>

#include <util/hashratecalculator.hpp>

namespace ses {
namespace proxy {

class ClientStatistics {
public:
  ClientStatistics();
  ClientStatistics(const std::string& username, const std::string& password, const std::string lastIp,
                   const util::HashRateCalculator& hashrate, uint64_t numTotalShares, uint64_t numGoodShares);

  std::string getUsername() const;
  void setUsername(const std::string& username);

  std::string getPassword() const;
  void setPassword(const std::string& password);

  std::string getLastIp() const;
  void setLastIp(const std::string& lastIp);

  uint16_t getMinerCount() const;
  void incrementMinerCount();

  uint64_t getSharesTotal() const;
  void addSharesTotal(uint64_t sharesTotal);

  uint64_t getSharesGood() const;
  void addSharesGood(uint64_t sharesGood);

  double getHashrateShort() const;
  double getHashrateMedium() const;
  double getHashrateLong() const;
  double getHashrateExtraLong() const;
  void addHashrate(const util::HashRateCalculator& hashrate);

  ClientStatistics& operator+=(const ClientStatistics& rhs);

  static void printHeading(std::ostream& out);
  friend std::ostream& operator<<(std::ostream& out, ClientStatistics const& stat);

private:
  std::string username_;
  std::string password_;
  std::string lastIp_;
  uint16_t minerCount_;
  uint64_t sharesTotal_;
  uint64_t sharesGood_;
  double hashrateShort_;     // 1min
  double hashrateMedium_;    // 60min
  double hashrateLong_;      // 12h
  double hashrateExtraLong_; // 24h
};

} // namespace proxy
} // namespace ses