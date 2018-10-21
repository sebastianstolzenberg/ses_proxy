#include <algorithm>
#include <ostream>

#include "util/hashratecalculator.hpp"

namespace ses {
namespace util {
namespace {
void updateAverageHashrate(double& averageHashRate, double hashRateLastUpdate, std::chrono::milliseconds timeWindow,
                           const std::chrono::milliseconds& timeSinceLastUpdate,
                           const std::chrono::milliseconds& timeSinceInit)
{
  if (averageHashRate == 0)
  {
    averageHashRate = hashRateLastUpdate;
  }
  else
  {
    double diffFractionOfShortTimeWindow = static_cast<double>(timeSinceLastUpdate.count()) /
                                           std::min(std::chrono::milliseconds(timeWindow), timeSinceInit).count();
    diffFractionOfShortTimeWindow = std::min(diffFractionOfShortTimeWindow, 1.0);
    averageHashRate = (averageHashRate * (1 - diffFractionOfShortTimeWindow)) +
                                      (hashRateLastUpdate * diffFractionOfShortTimeWindow);
  }
}
}

HashRateCalculator::HashRateCalculator(const std::chrono::seconds& shortTimeWindow,
                                       const std::chrono::seconds& mediumTimeWindow,
                                       const std::chrono::seconds& longTimeWindow,
                                       const std::chrono::seconds& extraLongTimeWindow)
  : shortTimeWindow_(shortTimeWindow), mediumTimeWindow_(mediumTimeWindow), longTimeWindow_(longTimeWindow),
    extraLongTimeWindow_(extraLongTimeWindow), initTimePoint_(std::chrono::system_clock::now()),
    lastUpdateTimePoint_(initTimePoint_), totalHashes_(0), hashRateLastUpdate_(0), hashRateAverageShortTimeWindow_(0),
    hashRateAverageLongTimeWindow_(0)
{
}

void HashRateCalculator::addHashes(uint32_t hashes)
{
  std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
  std::chrono::milliseconds timeSinceInit =
    std::chrono::duration_cast<std::chrono::milliseconds>(now - initTimePoint_);
  std::chrono::milliseconds timeSinceLastUpdate =
    std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTimePoint_);
  lastUpdateTimePoint_ = now;

  totalHashes_ += hashes;

  // prevent division by zero
  if (timeSinceInit.count() == 0 || timeSinceLastUpdate.count() == 0)
  {
    //TODO find a better approach
    return;
  }

  hashRateLastUpdate_ = static_cast<double>(hashes * 1000) / timeSinceLastUpdate.count();

  updateAverageHashrate(hashRateAverageShortTimeWindow_, hashRateLastUpdate_, shortTimeWindow_, timeSinceLastUpdate,
                        timeSinceInit);
  updateAverageHashrate(hashRateAverageMediumTimeWindow_, hashRateLastUpdate_, mediumTimeWindow_, timeSinceLastUpdate,
                        timeSinceInit);
  updateAverageHashrate(hashRateAverageLongTimeWindow_, hashRateLastUpdate_, longTimeWindow_, timeSinceLastUpdate,
                        timeSinceInit);
  updateAverageHashrate(hashRateAverageExtraLongTimeWindow_, hashRateLastUpdate_, extraLongTimeWindow_,
                        timeSinceLastUpdate, timeSinceInit);
}

void HashRateCalculator::addHashRate(uint32_t hashRate)
{
  uint32_t timeSinceLastUpdate =
    std::chrono::duration_cast<
      std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdateTimePoint_).count();
  addHashes(hashRate * timeSinceLastUpdate / 1000);
}

uint64_t HashRateCalculator::getTotalHashes() const
{
  return totalHashes_;
}

double HashRateCalculator::getHashRateLastUpdate() const
{
  return hashRateLastUpdate_;
}

double HashRateCalculator::getAverageHashRateShortTimeWindow() const
{
  return hashRateAverageShortTimeWindow_;
}

double HashRateCalculator::getAverageHashRateMediumTimeWindow() const
{
  return hashRateAverageMediumTimeWindow_;
}

double HashRateCalculator::getAverageHashRateLongTimeWindow() const
{
  return hashRateAverageLongTimeWindow_;
}

double HashRateCalculator::getAverageHashRateExtraLongTimeWindow() const
{
  return hashRateAverageExtraLongTimeWindow_;
}

std::chrono::seconds HashRateCalculator::secondsSinceStart() const
{
  return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - initTimePoint_);
}

std::ostream& operator<<(std::ostream& stream, const HashRateCalculator& hashrate)
{
  stream << "HashRates (short, " << hashrate.hashRateAverageShortTimeWindow_
         << ", medium, " << hashrate.hashRateAverageMediumTimeWindow_
         << ", long, " << hashrate.hashRateAverageLongTimeWindow_
         << ", extraLong, " << hashrate.hashRateAverageExtraLongTimeWindow_
         << ", total hashes, " << hashrate.totalHashes_ << ")";
}

HashRateCalculator& HashRateCalculator::operator+=(const HashRateCalculator& hashrate)
{
  hashRateAverageShortTimeWindow_ += hashrate.hashRateAverageShortTimeWindow_;
  hashRateAverageMediumTimeWindow_ += hashrate.hashRateAverageMediumTimeWindow_;
  hashRateAverageLongTimeWindow_ += hashrate.hashRateAverageLongTimeWindow_;
  hashRateAverageExtraLongTimeWindow_ += hashrate.hashRateAverageExtraLongTimeWindow_;
  totalHashes_ += hashrate.totalHashes_;
  
  return *this;
}

} // namespace util
} // namespace ses
