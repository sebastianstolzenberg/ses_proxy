#ifndef SES_PROXY_WORKER_HPP
#define SES_PROXY_WORKER_HPP

#include <memory>
#include <boost/noncopyable.hpp>

#include "proxy/job.hpp"
#include "proxy/workeridentifier.hpp"
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
  virtual void assignJob(const Job::Ptr& job) = 0;
};

} // namespace proxy
} // namespace ses

#endif //SES_PROXY_WORKER_HPP
