#pragma once

#include <mutex>
#include <map>
#include <boost/cstdint.hpp>

#include "proxy/workeridentifier.hpp"
#include "proxy/blob.hpp"
#include "proxy/jobtemplate.hpp"
#include "util/log.hpp"

namespace ses {
namespace proxy {

namespace {
std::string generateJobIdentifier()
{
  return boost::lexical_cast<std::string>(boost::uuids::random_generator()());
}
}

class BaseJobTemplate : public JobTemplate, public std::enable_shared_from_this<BaseJobTemplate>
{
public:
  BaseJobTemplate(const std::string& identifier, const std::string& jobIdentifier, const Algorithm& algorithm,
                  const Blob& blob)
    : identifier_(identifier), jobIdentifier_(jobIdentifier), algorithm_(algorithm), blob_(std::move(blob))
  {
  }

  void setJobResultHandler(const JobResult::Handler& jobResultHandler) override
  {
    jobResultHandler_ = jobResultHandler;
  }

  bool supportsWorkerType(WorkerType workerType) override
  {
    return workerType != WorkerType::PROXY;
  }

  Job::Ptr getJobFor(const WorkerIdentifier& workerIdentifier, WorkerType workerType) override
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    Job::Ptr job;
    auto subJobIt = subJobs_.find(workerIdentifier);
    if (subJobIt != subJobs_.end())
    {
      // erases last job assigned to the worker
      subJobs_.erase(subJobIt);
    }
    job = getNextSubJob(workerIdentifier, workerType);
    subJobs_[workerIdentifier] = job;
    return job;
  }

  void submitResult(const JobResult& result,
                    const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {
  }
  size_t numHashesFound() const override {return 0;}
  size_t currentHashRate() const override {return 0;}
  util::Target getTarget() const override {return util::Target(UINT32_C(0));}
  uint32_t getDifficulty() const override {return 0;}

  const std::string& getJobIdentifier() const override
  {
    return jobIdentifier_;
  }

  Algorithm getAlgorithm() const override 
  {
    return algorithm_;
  }

  stratum::Job asStratumJob() const override
  {
    return stratum::Job();
  }

protected:
  virtual Job::Ptr getNextSubJob(const WorkerIdentifier& workerIdentifier, WorkerType workerType) = 0;

protected:
  std::recursive_mutex mutex_;

  std::string identifier_;
  std::string jobIdentifier_;
  Algorithm algorithm_;
  Blob blob_;
  JobResult::Handler jobResultHandler_;

  std::map<WorkerIdentifier, Job::Ptr> subJobs_;
};

} // namespace proxy
} // namespace ses