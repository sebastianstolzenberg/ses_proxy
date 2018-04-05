#pragma once

#include <deque>

namespace ses {
namespace util {

template <class T>
class HashRateSampler
{
public:
  HashRateSampler(const typename T::Ptr& hasher) : hasher_(hasher) {}

  void sampleCurrentState()
  {
    hashRate_ = hasher_->getHashRate().getAverageHashRateLongTimeWindow();
  }

  bool operator<(const HashRateSampler<T>& other) const
  {
    return hashRate_ < other.hashRate_;
  }

  bool operator>(const HashRateSampler<T>& other) const
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
    std::sort(hashers_.begin(), hashers_.end(), std::greater<HashRateSampler<T> >());
  }

  std::deque<HashRateSampler<T> > hashers_;
  double accumulatedHashRate_;
};

template <class Producer>
class HashRateProducer : public HashRateSampler<Producer>
{
  HashRateProducer(const typename Producer::Ptr& producer) : HashRateSampler<Producer>(producer) {}
};

template <class Consumer, class Producer>
class HashRateConsumer : public HashRateSampler<Consumer>
{
  HashRateConsumer(const typename Consumer::Ptr& consumer) : HashRateSampler<Consumer>(consumer) {}

  double getWeight() const
  {
    return HashRateSampler<Consumer>::hasher_->getWeight();
  }

  void setTargetHashRate(double totalAvailableHashRate)
  {
    //TODO add deviation from average
    remainingTargetHashRate_ = totalAvailableHashRate * getWeight();
  }

  bool assignProducer(HashRateProducer<Producer>& producer)
  {
    bool assigned = HashRateSampler<Consumer>::hasher_->addWorker(producer);
    if (assigned)
    {
      remainingTargetHashRate_ -= producer->hashRate_;
    }
    return assigned;
  }

  double remainingTargetHashRate_;
};

template <class Producer, class Consumer>
void rebalance(HashRateCollector<From>& provider, HashRateCollector<To>& receiver)
{
  // samples the current state
  provider.sampleCurrentState();
  receiver.sampleCurrentState();



}


} // namespace util
} // namespace ses
