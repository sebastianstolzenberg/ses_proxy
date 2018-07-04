#pragma once

#include <chrono>

namespace ses {
namespace util {

class HashRateCalculator
{
public:
  HashRateCalculator(const std::chrono::seconds& shortTimeWindow = std::chrono::minutes(1),
                     const std::chrono::seconds& mediumTimeWindow = std::chrono::hours(1),
                     const std::chrono::seconds& longTimeWindow = std::chrono::hours(12),
                     const std::chrono::seconds& extraLongTimeWindow = std::chrono::hours(24));

  void addHashes(uint32_t hashes);
  void addHashRate(uint32_t hashRate);

  uint64_t getTotalHashes() const;
  double getHashRateLastUpdate() const;
  double getAverageHashRateShortTimeWindow() const;
  double getAverageHashRateMediumTimeWindow() const;
  double getAverageHashRateLongTimeWindow() const;
  double getAverageHashRateExtraLongTimeWindow() const;

  std::chrono::seconds secondsSinceStart() const;

  friend std::ostream& operator<<(std::ostream& stream, const HashRateCalculator& hashrate);

private:
  std::chrono::seconds shortTimeWindow_;
  std::chrono::seconds mediumTimeWindow_;
  std::chrono::seconds longTimeWindow_;
  std::chrono::seconds extraLongTimeWindow_;

  std::chrono::time_point<std::chrono::system_clock> initTimePoint_;
  std::chrono::time_point<std::chrono::system_clock> lastUpdateTimePoint_;

  uint64_t totalHashes_;
  double hashRateLastUpdate_;
  double hashRateAverageShortTimeWindow_;
  double hashRateAverageMediumTimeWindow_;
  double hashRateAverageLongTimeWindow_;
  double hashRateAverageExtraLongTimeWindow_;
};

} // namespace util
} // namespace ses
