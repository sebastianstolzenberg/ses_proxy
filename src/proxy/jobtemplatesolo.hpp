#pragma once

#include "proxy/jobtemplatebase.hpp"

namespace ses {
namespace proxy {

// For pool jobs which can't be broken down any further
// Only one single job can be retrieved from this template
class SoloJobTemplate : public BaseJobTemplate
{
public:
  SoloJobTemplate(const WorkerIdentifier& identifier, const std::string& jobIdentifier,
                  const Blob& blob, uint64_t target)
    : BaseJobTemplate(identifier, jobIdentifier, blob), target_(target)
  {
  }

  void submitResult(const JobResult& result,
                            const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {

  }

  void toStream(std::ostream& stream) const override
  {
    stream << "SoloJobTemplate, jobId, " << jobIdentifier_;
  }

protected:
  Job::Ptr getNextSubJob(const WorkerIdentifier& workerIdentifier, WorkerType workerType) override
  {
    Job::Ptr job;
    if (workerType != WorkerType::PROXY)
    {
      if (subJobs_.empty())
      {
        JobResult::Handler resultHandler =
          std::bind(&SoloJobTemplate::handleResult,
                    std::dynamic_pointer_cast<SoloJobTemplate>(shared_from_this()),
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        job = Job::createMinerJob(workerIdentifier, generateJobIdentifier(), blob_, target_, resultHandler);
        //TODO connect ResultHandler
      }
    }
    return job;
  }

  void handleResult(const WorkerIdentifier& workerIdentifier,
                    const JobResult& jobResult,
                    const JobResult::SubmitStatusHandler& submitStatusHandler)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
//    LOG_DEBUG << __PRETTY_FUNCTION__;

    if (foundNonces_.count(jobResult.getNonce()) > 0)
    {
      submitStatusHandler(JobResult::SUBMIT_REJECTED_DUPLICATE);
    }
    else if (jobResult.getNiceHash() != blob_.getNiceHash())
    {
      // "Malformed nonce";
      submitStatusHandler(JobResult::SUBMIT_REJECTED_DUPLICATE);
    }
    else if (jobResultHandler_)
    {
      submitStatusHandler(JobResult::SUBMIT_REJECTED_INVALID_JOB_ID);
    }
    else
    {
      JobResult modifiedResult = jobResult;
      modifiedResult.setJobId(jobIdentifier_);
      JobResult::SubmitStatusHandler soloJobTemplateSubmitStatusHandler =
        std::bind(&SoloJobTemplate::handleSubmitStatus,
                  std::dynamic_pointer_cast<SoloJobTemplate>(shared_from_this()),
                  std::placeholders::_1, submitStatusHandler, modifiedResult.getNonce());
      jobResultHandler_(identifier_, modifiedResult, soloJobTemplateSubmitStatusHandler);
    }
  }

  void handleSubmitStatus(JobResult::SubmitStatus submitStatus, JobResult::SubmitStatusHandler handler, uint32_t nonce)
  {
    LOG_TRACE << __PRETTY_FUNCTION__ << " " << submitStatus << " " << nonce;
    if (submitStatus == JobResult::SUBMIT_ACCEPTED)
    {
      foundNonces_.insert(nonce);
    }
    handler(submitStatus);
  }

private:
  uint64_t target_;
  std::set<uint32_t> foundNonces_;
};

} // namespace proxy
} // namespace ses