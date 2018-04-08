#pragma once

#include <deque>
#include <boost/range/value_type.hpp>

namespace ses {
namespace util {
namespace hashrate {

namespace {
template <typename V> inline V& getValue(V& v) { return v; }
template <typename K, typename V> inline V& getValue(std::pair<K, V>& p) { return p.second; }
}

template <class T>
class Sampler
{
public:
  Sampler(const T& hasher) : hasher_(hasher) {}

  void sampleCurrentState()
  {
    hashRate_ = getValue(hasher_)->getHashRate().getAverageHashRateLongTimeWindow();
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

  T hasher_;

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
    double targetHashRate_ = totalAvailableHashRate * getWeight();
    double actualAverage =
      Sampler<ConsumerImplementation>::hasher_->getWorkerHashRate().getAverageHashRateLongTimeWindow();
    // substracts current deviation of the average from the ideal average
    remainingTargetHashRate_ = 2 * targetHashRate_ - actualAverage;
    std::cout << "(t" << targetHashRate_ << ",a" << actualAverage << ",r" << remainingTargetHashRate_ << ")";
  }

  template <class ProducerImplementation>
  bool assignProducer(Producer<ProducerImplementation>& producer)
  {
    bool assigned = Sampler<ConsumerImplementation>::hasher_->addWorker(producer.hasher_);
    if (assigned)
    {
      remainingTargetHashRate_ -= producer.hashRate_;
    }
    return assigned;
  }

  template <class ProducerImplementation>
  bool releaseProducer(Producer<ProducerImplementation>& producer)
  {
    Sampler<ConsumerImplementation>::hasher_->removeWorker(producer.hasher_);
  }

  double remainingTargetHashRate_;
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


template <class ConsumerImplementation>
class ConsumerPool
{
public:
  template <class ConsumerContainer>
  ConsumerPool(ConsumerContainer& container)
    : consumers_(container.begin(), container.end())
  {
  }

  void determineTargetHashRates(double totalAvailableHashRate)
  {
    for (auto& consumer : consumers_)
    {
      consumer.setTargetHashRate(totalAvailableHashRate);
    }
    std::cout << std::endl;
  }

  void sortByRemainingHashRateDescending()
  {
    std::sort(consumers_.begin(), consumers_.end(),
              std::greater<Consumer<ConsumerImplementation> >());
  }

  void releaseProducers()
  {
    for (auto& consumer : consumers_)
    {
      getValue(consumer.hasher_)->removeAllWorkers();
    }
  }

  template <class ProducerImplementation>
  void distributeProducers(ProducerPool<ProducerImplementation>& producers)
  {
    if (!consumers_.empty())
    {
      determineTargetHashRates(producers.average_.accumulatedHashRate_);
      for (auto& producer : producers.producers_)
      {
        sortByRemainingHashRateDescending();
        bool assigned = false;
        for (auto& consumer : consumers_)
        {
          if (!assigned)
          {
            // tries to assign only if not yet assigned
            assigned = consumer.assignProducer(producer);
          }
          else
          {
            consumer.releaseProducer(producer);
          }
        }
      }
      //TODO handle producer which have not been accepted by any of the consumers
    }
  }

  std::deque<Consumer<ConsumerImplementation> > consumers_;
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

template <class ProducerContainer, class ConsumerContainer>
void rebalance(ProducerContainer& producer, ConsumerContainer& consumer)
{
  ProducerPool<typename ProducerContainer::value_type> producerPool(producer);

  ConsumerPool<typename ConsumerContainer::value_type> consumerPool(consumer);

  // samples the current state
  producerPool.sampleCurrentState();
  consumerPool.distributeProducers(producerPool);
}

} // namespace hashrate
} // namespace util
} // namespace ses
