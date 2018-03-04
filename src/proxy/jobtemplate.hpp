#pragma once

#include <memory>
#include <functional>

#include "stratum/job.hpp"
#include "proxy/job.hpp"
#include "proxy/workeridentifier.hpp"
#include "proxy/workertype.hpp"

namespace ses {
namespace proxy {

class JobTemplate : public Job
{
public:
  typedef std::shared_ptr<JobTemplate> Ptr;

public:
  static JobTemplate::Ptr create(const stratum::Job& stratumJob);

  virtual void setJobResultHandler(const JobResult::Handler& jobResultHandler) = 0;

  //TODO set difficulty
  virtual bool supportsWorkerType(WorkerType workerType) = 0;
  virtual Job::Ptr getJobFor(const WorkerIdentifier& workerIdentifier, WorkerType workerType) = 0;

protected:
  JobTemplate() = default;
  virtual ~JobTemplate() = default;
};

} // namespace proxy
} // namespace ses
