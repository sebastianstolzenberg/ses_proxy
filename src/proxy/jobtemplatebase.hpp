#pragma once

#include <mutex>
#include <map>

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
  BaseJobTemplate(const WorkerIdentifier& identifier, const std::string& jobIdentifier, const Blob& blob)
    : identifier_(identifier), jobIdentifier_(jobIdentifier), blob_(std::move(blob))
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
    if (subJobIt == subJobs_.end())
    {
      // new worker
      job = getNextSubJob(workerIdentifier, workerType);
      subJobs_[workerIdentifier] = job;
    }
    else
    {
      job = subJobIt->second;
    }
    return job;
  }

  void submitResult(const JobResult& result,
                    const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {
  }
  size_t numHashesFound() const override {return 0;}
  size_t currentHashRate() const override {return 0;}
  const std::string& getJobIdentifier() const override
  {
    return jobIdentifier_;
  }

  stratum::Job asStratumJob() const override
  {
    return stratum::Job();
  }

protected:
  virtual Job::Ptr getNextSubJob(const WorkerIdentifier& workerIdentifier, WorkerType workerType) = 0;

protected:
  std::recursive_mutex mutex_;

  WorkerIdentifier identifier_;
  std::string jobIdentifier_;
  Blob blob_;
  JobResult::Handler jobResultHandler_;

  std::map<WorkerIdentifier, Job::Ptr> subJobs_;
};

} // namespace proxy
} // namespace ses