#pragma once

namespace ses {
namespace proxy {

class ClientStat {
public:
  ClientStat()
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