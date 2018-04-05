#pragma once

#include <deque>

namespace ses {
namespace util {

template <class T>
class HashRateTracker
{
public:
  HashRateTracker(const typename T::Ptr& hasher) : hasher_(hasher) {}

  void sampleCurrentState()
  {
    hashRate_ = hasher_->getHashRate().getAverageHashRateLongTimeWindow();
  }

  bool operator<(const HashRateTracker<T>& other) const
  {
    return hashRate_ < other.hashRate_;
  }

  bool operator>(const HashRateTracker<T>& other) const
  {
    return hashRate_ > other.hashRate_;
  }

  bool operator==(const typename T::Ptr& hasher) const
  {
    return hasher_ == hasher;
  }

  typename T::Ptr hasher_;

  double hashRate_;
};

template <class T>
class HashRateCollector
{
public:
  void addHasher(const typename T::Ptr& hasher)
  {
    hashers_.emplace_back(hasher);
  }

  void removeHasher(const typename T::Ptr& client)
  {
    auto it = std::find(hashers_.begin(), hashers_.end(), client);
    if (it != hashers_.end())
    {
      hashers_.erase(it);
    }
  }

  void sampleCurrentState()
  {
    accumulatedHashRate_ = 0;
    for (auto& hasher : hashers_)
    {
      hasher.sampleCurrentState();
      accumulatedHashRate_ += hasher.hashRate_;
    }
    std::sort(hashers_.begin(), hashers_.end(), std::greater<HashRateTracker<T> >());
  }

  std::deque<HashRateTracker<T> > hashers_;
  double accumulatedHashRate_;
};

class HashRateConsumer
{

};

template <class From, class To>
void rebalance(HashRateCollector<From>& provider, HashRateCollector<To>& receiver)
{
  provider.sampleCurrentState();
  receiver.sampleCurrentState();


}


} // namespace util
} // namespace ses
