#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <numeric>

#include "stratum/job.hpp"
#include "proxy/workeridentifier.hpp"
#include "proxy/jobresult.hpp"
#include "proxy/blob.hpp"

namespace ses {
namespace proxy {

class Job
{
public:
  typedef std::shared_ptr<Job> Ptr;

public:
  Job::Ptr createMinerJob(const WorkerIdentifier& workerIdentifier, const std::string& jobIdentifier,
                          const Blob& blob, uint32_t target, const JobResult::Handler& jobResultHandler);

  virtual ~Job() = default;

  virtual void submitResult(const JobResult& result,
                            const JobResult::SubmitStatusHandler& submitStatusHandler) = 0;

  virtual size_t numHashesFound() const = 0;
  virtual size_t currentHashRate() const = 0;
  virtual const std::string& getJobIdentifier() const = 0;
  virtual stratum::Job asStratumJob() const = 0;

protected:
  Job() = default;
};

} // namespace proxy
} // namespace ses
