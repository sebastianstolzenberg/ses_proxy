#pragma once

namespace ses {
namespace proxy {

class ClientStatistics {
public:
  ClientStatistics()
    : minerCount_(0),
      sharesTotal_(0),
      sharesGood_(0),
      hashrateShort_(0),
      hashrateMedium_(0),
      hashrateLong_(0),
      hashrateExtraLong_(0) {
  }

  std::string getUsername() const {
    return username_;
  }

  void setUsername(std::string username) {
    username_ = username;
  }

  std::string getLastIp() const {
    return lastIp_;
  }

  void setLastIp(std::string lastIp) {
    lastIp_ = lastIp;
  }

  uint16_t getMinerCount() const {
    return minerCount_;
  }

  void incrementMinerCount() {
    minerCount_++;
  }

  uint64_t getSharesTotal() const {
    return sharesTotal_;
  }

  void addSharesTotal(uint64_t sharesTotal) {
    sharesTotal_ += sharesTotal;
  }

  uint64_t getSharesGood() const {
    return sharesGood_;
  }

  void addSharesGood(uint64_t sharesGood) {
    sharesGood_ += sharesGood;
  }

  double getHashrateShort() const {
    return hashrateShort_;
  }

  void addHashrateShort(double hashrateShort) {
    hashrateShort_ += hashrateShort;
  }

  double getHashrateMedium() const {
    return hashrateMedium_;
  }

  void addHashrateMedium(double hashrateMedium) {
    hashrateMedium_ += hashrateMedium;
  }

  double getHashrateLong() const {
    return hashrateLong_;
  }

  void addHashrateLong(double hashrateLong) {
    hashrateLong_ += hashrateLong;
  }

  double getHashrateExtraLong() const {
    return hashrateExtraLong_;
  }

  void addHashrateExtraLong(double hashrateExtraLong) {
    hashrateExtraLong_ += hashrateExtraLong;
  }

  static void printHeading(std::ostream& out)
  {
    out << " | "
        << std::left << std::setw(12) << "User" << std::setw(2) << "| "
        << std::left << std::setw(15) << "Last IP" << std::setw(2) << "| "
        << std::right << std::setw(8) << "Miners" << std::setw(2) << "| "
        << std::right << std::setw(12) << "Shares" << std::setw(2) << "| "
        << std::right << std::setw(12) << "Rejected" << std::setw(2) << "| "
        << std::right << std::setw(14) << "Hashrate (1m)" << std::setw(2) << "| "
        << std::right << std::setw(14) << "Hashrate (60m)" << std::setw(2) << "| "
        << std::right << std::setw(14) << "Hashrate (12h)" << std::setw(2) << "| "
        << std::right << std::setw(14) << "Hashrate (24h)" << std::setw(2) << "| "
        << std::endl;
  }

  friend std::ostream& operator<<(std::ostream& out, ClientStatistics const& stat)
  {
    out << " | "
        << std::left << std::setw(12) << stat.getUsername().substr(0, 11) << std::setw(2) << "| "
        << std::left << std::setw(15) << stat.getLastIp() << std::setw(2) << "| "
        << std::right << std::setw(8) << stat.getMinerCount() << std::setw(2) << "| "
        << std::right << std::setw(12) << stat.getSharesGood() << std::setw(2) << "| "
        << std::right << std::setw(12) << stat.getSharesTotal() - stat.getSharesGood() << std::setw(2) << "| "
        << std::right << std::setw(14) << stat.getHashrateShort() << std::setw(2) << "| "
        << std::right << std::setw(14) << stat.getHashrateMedium() << std::setw(2) << "| "
        << std::right << std::setw(14) << stat.getHashrateLong() << std::setw(2) << "| "
        << std::right << std::setw(14) << stat.getHashrateExtraLong() << std::setw(2) << "| "
        << std::endl;
    return out;
  }

private:
  std::string username_;
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