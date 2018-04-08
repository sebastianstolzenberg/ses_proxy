#pragma once

#include <chrono>

namespace ses {
namespace util {

class HashRateCalculator
{
public:
  HashRateCalculator(const std::chrono::seconds& shortTimeWindow = std::chrono::seconds(60),
                     const std::chrono::seconds& longTimeWindow = std::chrono::seconds(600));

  void addHashes(uint32_t hashes);
  void addHashRate(uint32_t hashRate);

  uint32_t getTotalHashes() const;
  double getHashRateLastUpdate() const;
  double getAverageHashRateShortTimeWindow() const;
  double getAverageHashRateLongTimeWindow() const;

  std::chrono::seconds secondsSinceStart() const;

  friend std::ostream& operator<<(std::ostream& stream, const HashRateCalculator& hashrate);

private:
  std::chrono::seconds shortTimeWindow_;
  std::chrono::seconds longTimeWindow_;

  std::chrono::time_point<std::chrono::system_clock> initTimePoint_;
  std::chrono::time_point<std::chrono::system_clock> lastUpdateTimePoint_;

  uint32_t totalHashes_;
  double hashRateLastUpdate_;
  double hashRateAverageShortTimeWindow_;
  double hashRateAverageLongTimeWindow_;
};

} // namespace util
} // namespace ses
