#pragma once

#include <deque>
#include <boost/range/value_type.hpp>

namespace ses {
namespace util {
namespace hashrate {

template <class ProducerImplementation>
class Producer
{
public:
  Producer(const ProducerImplementation& producer) : producer_(producer) {}

  const ProducerImplementation& get() {return producer_;}

  bool operator<(const Producer<ProducerImplementation>& other) const
  {
    return hashRate_ < other.hashRate_;
  }

  bool operator>(const Producer<ProducerImplementation>& other) const
  {
    return hashRate_ > other.hashRate_;
  }

  void sampleCurrentState()
  {
    hashRate_ = producer_->getHashRate().getAverageHashRateLongTimeWindow();
  }

  ProducerImplementation producer_;
  double hashRate_;
};

template <class ConsumerImplementation>
class Consumer
{
public:
  Consumer(const ConsumerImplementation& consumer) : consumer_(consumer) {}

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
    return consumer_->getWeight();
  }

  void setTargetHashRate(double totalAvailableHashRate)
  {
    double targetHashRate_ = totalAvailableHashRate * getWeight();
    double actualAverage = consumer_->getWorkerHashRate().getAverageHashRateLongTimeWindow();
    // substracts current deviation of the average from the ideal average
    remainingTargetHashRate_ = 2 * targetHashRate_ - actualAverage;
    std::cout << "(t" << targetHashRate_ << ",a" << actualAverage << ",r" << remainingTargetHashRate_ << ")";
  }

  template <class ProducerImplementation>
  bool assignProducer(Producer<ProducerImplementation>& producer)
  {
    bool assigned = consumer_->addWorker(producer.get());
    if (assigned)
    {
      remainingTargetHashRate_ -= producer.hashRate_;
    }
    return assigned;
  }

  template <class ProducerImplementation>
  bool releaseProducer(Producer<ProducerImplementation>& producer)
  {
    consumer_->removeWorker(producer.get());
  }

  ConsumerImplementation consumer_;
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
    accumulatedHashRate_ = 0;
    for (auto& hasher : producers_)
    {
      hasher.sampleCurrentState();
      accumulatedHashRate_ += hasher.hashRate_;
    }
    std::sort(producers_.begin(), producers_.end(),
              std::greater<Producer<ProducerImplementation> >());
  }

  std::deque<Producer<ProducerImplementation> > producers_;
  double accumulatedHashRate_;
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

  template <class ProducerImplementation>
  void distributeProducers(ProducerPool<ProducerImplementation>& producers)
  {
    if (!consumers_.empty())
    {
      determineTargetHashRates(producers.accumulatedHashRate_);
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
