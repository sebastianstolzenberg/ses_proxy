#include <iomanip>

#include "proxy/clientstatistics.hpp"

namespace ses {
namespace proxy {

ClientStatistics::ClientStatistics()
  : minerCount_(0), sharesTotal_(0), sharesGood_(0), hashrateShort_(0), hashrateMedium_(0), hashrateLong_(0)
    , hashrateExtraLong_(0)
{
}

ClientStatistics::ClientStatistics(const std::string& username, const std::string& password, const std::string lastIp,
                                   const util::HashRateCalculator& hashrate, uint64_t numTotalShares,
                                   uint64_t numGoodShares)
  : username_(username), password_(password), lastIp_(lastIp), minerCount_(1), sharesTotal_(numTotalShares),
    sharesGood_(numGoodShares),
    hashrateShort_(hashrate.getAverageHashRateShortTimeWindow()),
    hashrateMedium_(hashrate.getAverageHashRateMediumTimeWindow()),
    hashrateLong_(hashrate.getAverageHashRateLongTimeWindow()),
    hashrateExtraLong_(hashrate.getAverageHashRateExtraLongTimeWindow())
{
}

std::string ClientStatistics::getUsername() const
{
  return username_;
}

void ClientStatistics::setUsername(const std::string& username)
{
  username_ = username;
}

std::string ClientStatistics::getPassword() const
{
  return password_;
}

void ClientStatistics::setPassword(const std::string& password)
{
  password_ = password;
}

std::string ClientStatistics::getLastIp() const
{
  return lastIp_;
}

void ClientStatistics::setLastIp(const std::string& lastIp)
{
  lastIp_ = lastIp;
}

uint16_t ClientStatistics::getMinerCount() const
{
  return minerCount_;
}

void ClientStatistics::incrementMinerCount()
{
  minerCount_++;
}

uint64_t ClientStatistics::getSharesTotal() const
{
  return sharesTotal_;
}

void ClientStatistics::addSharesTotal(uint64_t sharesTotal)
{
  sharesTotal_ += sharesTotal;
}

uint64_t ClientStatistics::getSharesGood() const
{
  return sharesGood_;
}

void ClientStatistics::addSharesGood(uint64_t sharesGood)
{
  sharesGood_ += sharesGood;
}

double ClientStatistics::getHashrateShort() const
{
  return hashrateShort_;
}

double ClientStatistics::getHashrateMedium() const
{
  return hashrateMedium_;
}

double ClientStatistics::getHashrateLong() const
{
  return hashrateLong_;
}

double ClientStatistics::getHashrateExtraLong() const
{
  return hashrateExtraLong_;
}

void ClientStatistics::addHashrate(const util::HashRateCalculator& hashrate)
{
  hashrateShort_ += hashrate.getAverageHashRateShortTimeWindow();
  //TODO medium
  hashrateLong_ += hashrate.getAverageHashRateLongTimeWindow();
  //TODO extra long
}

ClientStatistics& ClientStatistics::operator+=(const ClientStatistics& rhs)
{
  username_ = rhs.username_;
  password_ = rhs.password_;
  lastIp_ = rhs.lastIp_;
  minerCount_ += rhs.minerCount_;
  sharesTotal_ += rhs.sharesTotal_;
  sharesGood_ += rhs.sharesGood_;
  hashrateShort_ += rhs.hashrateShort_;
  hashrateMedium_ += rhs.hashrateMedium_;
  hashrateLong_ += rhs.hashrateLong_;
  hashrateExtraLong_ += rhs.hashrateExtraLong_;
  return *this;
}

void ClientStatistics::printHeading(std::ostream& out)
{
  out << " | "
      << std::left << std::setw(12) << "User" << std::setw(2) << "| "
      << std::left << std::setw(12) << "Password" << std::setw(2) << "| "
      << std::left << std::setw(15) << "Last IP" << std::setw(2) << "| "
      << std::left << std::setw(8) << "Miners" << std::setw(2) << "| "
      << std::left << std::setw(12) << "Good Shares" << std::setw(2) << "| "
      << std::left << std::setw(12) << "Total Shares" << std::setw(2) << "| "
      << std::left << std::setw(14) << "Hashrate (1m)" << std::setw(2) << "| "
      << std::left << std::setw(14) << "Hashrate (60m)" << std::setw(2) << "| "
      << std::left << std::setw(14) << "Hashrate (12h)" << std::setw(2) << "| "
      << std::left << std::setw(14) << "Hashrate (24h)" << std::setw(2) << "| "
      << std::endl;
}

std::ostream& operator<<(std::ostream& out, ClientStatistics const& stat)
{
  out << " | "
      << std::left << std::setw(12) << stat.getUsername().substr(0, 11) << std::setw(2) << "| "
      << std::left << std::setw(12) << stat.getPassword().substr(0, 11) << std::setw(2) << "| "
      << std::left << std::setw(15) << stat.getLastIp() << std::setw(2) << "| "
      << std::right << std::setw(8) << stat.getMinerCount() << std::setw(2) << "| "
      << std::right << std::setw(12) << stat.getSharesGood() << std::setw(2) << "| "
      << std::right << std::setw(12) << stat.getSharesTotal() << std::setw(2) << "| "
      << std::right << std::setw(14) << stat.getHashrateShort() << std::setw(2) << "| "
      << std::right << std::setw(14) << stat.getHashrateMedium() << std::setw(2) << "| "
      << std::right << std::setw(14) << stat.getHashrateLong() << std::setw(2) << "| "
      << std::right << std::setw(14) << stat.getHashrateExtraLong() << std::setw(2) << "| "
      << std::endl;
  return out;
}

} // namespace proxy
} // namespace ses