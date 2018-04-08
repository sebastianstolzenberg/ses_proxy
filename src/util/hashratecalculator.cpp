#include <algorithm>
#include <ostream>

#include "util/hashratecalculator.hpp"

namespace ses {
namespace util {

HashRateCalculator::HashRateCalculator(const std::chrono::seconds& shortTimeWindow,
                                       const std::chrono::seconds& longTimeWindow)
  : shortTimeWindow_(shortTimeWindow), longTimeWindow_(longTimeWindow),
    initTimePoint_(std::chrono::system_clock::now()), lastUpdateTimePoint_(initTimePoint_),
    totalHashes_(0), hashRateLastUpdate_(0), hashRateAverageShortTimeWindow_(0), hashRateAverageLongTimeWindow_(0)
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

  if (hashRateAverageShortTimeWindow_ == 0)
  {
    hashRateAverageShortTimeWindow_ = hashRateLastUpdate_;
  }
  else
  {
    double diffFractionOfShortTimeWindow = static_cast<double>(timeSinceLastUpdate.count()) /
                                   std::min(std::chrono::milliseconds(shortTimeWindow_), timeSinceInit).count();
    diffFractionOfShortTimeWindow = std::min(diffFractionOfShortTimeWindow, 1.0);
    hashRateAverageShortTimeWindow_ = (hashRateAverageShortTimeWindow_ * (1 - diffFractionOfShortTimeWindow)) +
                                      (hashRateLastUpdate_ * diffFractionOfShortTimeWindow);
  }

  if (hashRateAverageLongTimeWindow_ == 0)
  {
    hashRateAverageLongTimeWindow_ = hashRateLastUpdate_;
  }
  else
  {
    double diffFractionOfLongTimeWindow = static_cast<double>(timeSinceLastUpdate.count()) /
                                           std::min(std::chrono::milliseconds(longTimeWindow_), timeSinceInit).count();
    diffFractionOfLongTimeWindow = std::min(diffFractionOfLongTimeWindow, 1.0);
    hashRateAverageLongTimeWindow_ = (hashRateAverageLongTimeWindow_ * (1 - diffFractionOfLongTimeWindow)) +
                                     (hashRateLastUpdate_ * diffFractionOfLongTimeWindow);
  }
}

void HashRateCalculator::addHashRate(uint32_t hashRate)
{
  uint32_t timeSinceLastUpdate =
    std::chrono::duration_cast<
      std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdateTimePoint_).count();
  addHashes(hashRate * timeSinceLastUpdate / 1000);
}

uint32_t HashRateCalculator::getTotalHashes() const
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

double HashRateCalculator::getAverageHashRateLongTimeWindow() const
{
  return hashRateAverageLongTimeWindow_;
}

std::chrono::seconds HashRateCalculator::secondsSinceStart() const
{
  return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - initTimePoint_);
}

std::ostream& operator<<(std::ostream& stream, const HashRateCalculator& hashrate)
{
  stream << "HashRates: lastUpdate, " << hashrate.hashRateLastUpdate_
         << ", shortTimeWindow, " << hashrate.hashRateAverageShortTimeWindow_
         << ", longTimeWindow, " << hashrate.hashRateAverageLongTimeWindow_
         << ", total, " << hashrate.totalHashes_;
}

} // namespace util
} // namespace ses
