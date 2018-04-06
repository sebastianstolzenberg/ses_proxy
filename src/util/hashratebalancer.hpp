#pragma once

#include <deque>
#include <boost/range/value_type.hpp>

namespace ses {
namespace util {
namespace hashrate {

template <class T>
class Sampler
{
public:
  explicit Sampler(const std::shared_ptr<T>& hasher) : hasher_(hasher) {}

  template <typename PairFirst>
  Sampler(std::pair<PairFirst, std::shared_ptr<T> >& hasher) : hasher_(hasher.second) {}

  void sampleCurrentState()
  {
    hashRate_ = hasher_->getHashRate().getAverageHashRateLongTimeWindow();
  }

  bool operator<(const Sampler<T>& other) const
  {
    return hashRate_ < other.hashRate_;
  }

  bool operator>(const Sampler<T>& other) const
  {
    return hashRate_ > other.hashRate_;
  }

  bool operator==(const T& hasher) const
  {
    return hasher_ == hasher;
  }

  std::shared_ptr<T> hasher_;

  double hashRate_;
};

class Averager
{
public:
  template <class HashRateSamplerRange>
  void sampleCurrentState(HashRateSamplerRange& samplerRange)
  {
    accumulatedHashRate_ = 0;
    for (auto& hasher : samplerRange)
    {
      hasher.sampleCurrentState();
      accumulatedHashRate_ += hasher.hashRate_;
    }
    std::sort(samplerRange.begin(), samplerRange.end(),
              std::greater<typename HashRateSamplerRange::value_type >());
  }

  operator double() const
  {
    return accumulatedHashRate_;
  }

  double accumulatedHashRate_;
};

template <class ProducerImplementation>
class Producer : public Sampler<ProducerImplementation>
{
public:
  Producer(const ProducerImplementation& producer)
      : Sampler<ProducerImplementation>(producer) {}
};

template <class ConsumerImplementation>
class Consumer : public Sampler<ConsumerImplementation>
{
public:
  Consumer(const ConsumerImplementation& consumer)
      : Sampler<ConsumerImplementation>(consumer) {}

  bool operator<(const Consumer<ConsumerImplementation>& other) const
  {
    return remainingTargetHashRate_ < other.remainingTargetHashRate_;
  }

  bool operator>(const Consumer<ConsumerImplementation>& other) const
  {
    return remainingTargetHashRate_ > other.remainingTargetHashRate_;
  }

  double getWeight() const
  {
    return Sampler<ConsumerImplementation>::hasher_->getWeight();
  }

  void setTargetHashRate(double totalAvailableHashRate)
  {
    //TODO add deviation from average
    remainingTargetHashRate_ = totalAvailableHashRate * getWeight();
  }

  template <class ProducerImplementation>
  bool assignProducer(ProducerImplementation& producer)
  {
    bool assigned = Sampler<ConsumerImplementation>::hasher_->addWorker(producer);
    if (assigned)
    {
      remainingTargetHashRate_ -= producer->hashRate_;
    }
    return assigned;
  }

  double remainingTargetHashRate_;
};

template <class ConsumerImplementation>
class ConsumerPool
{
public:
  template <class ConsumerContainer>
  ConsumerPool(ConsumerContainer& container)
    : consumers_(container.begin(), container.end())
  {
  }

  void sortByRemainingHashRateDescending()
  {
    std::sort(consumers_.begin(), consumers_.end(),
              std::greater<Consumer<ConsumerImplementation> >());
  }

  std::deque<Consumer<ConsumerImplementation> > consumers_;
};

template <class ProducerImplementation>
class ProducerPool
{
public:
  template <class ProducerContainer>
  ProducerPool(ProducerContainer& container)
      : producers_(container.begin(), container.end())
  {
  }

  void sampleCurrentState()
  {
    average_.sampleCurrentState(producers_);
  }

  std::deque<Producer<ProducerImplementation> > producers_;
  Averager average_;
};

template <class ProducerContainer, class ProducerImplementation>
ProducerPool<ProducerImplementation> convertToProducerPool(ProducerContainer& producer);

template <class ProducerContainer>
ProducerPool<typename ProducerContainer::mapped_type> convertToProducerPool(ProducerContainer& producer)
{
  return ProducerPool<typename ProducerContainer::mapped_type>(producer);
};

template <class ProducerContainer>
ProducerPool<typename ProducerContainer::value_type> convertToProducerPool(ProducerContainer& producer)
{
  return ProducerPool<typename ProducerContainer::value_type>(producer);
};

template<typename T>
struct extract_value_type
{
  typedef T value_type;
};

template<template<typename, typename ...> class X, typename T, typename ...Args>
struct extract_value_type<X<T, Args...>>   //specialization
{
  typedef T value_type;
};

template <class ProducerContainer, class ConsumerContainer>
void rebalance(ProducerContainer& producer, ConsumerContainer& consumer)
{
  ProducerPool<extract_value_type<ProducerContainer>::value_type> producerPool(producer);



//  ConsumerPool<typename ConsumerContainer::value_type> consumerPool(consumer);

  // samples the current state
  producerPool.sampleCurrentState();
}

} // namespace hashrate
} // namespace util
} // namespace ses
