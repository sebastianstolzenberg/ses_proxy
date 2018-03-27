#pragma once

#include <memory>
#include <boost/noncopyable.hpp>

#include "util/hashratecalculator.hpp"
#include "proxy/job.hpp"
#include "proxy/jobtemplate.hpp"
#include "proxy/workeridentifier.hpp"
#include "proxy/workertype.hpp"
#include "proxy/algorithm.hpp"

namespace ses {
namespace proxy {

class Worker : private boost::noncopyable
{
public:
  typedef std::shared_ptr<Worker> Ptr;

protected:
  Worker() = default;
  virtual ~Worker() = default;

public:
  virtual WorkerIdentifier getIdentifier() const = 0;
  virtual Algorithm getAlgorithm() const = 0;
  virtual WorkerType getType() const = 0;
  virtual void assignJob(const Job::Ptr& job) = 0;

  virtual bool isConnected() const = 0;
  virtual const util::HashRateCalculator& getHashRate() const = 0;
};

} // namespace proxy
} // namespace ses
