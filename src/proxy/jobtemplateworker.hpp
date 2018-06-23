#pragma once

#include "proxy/jobtemplatebase.hpp"
#include "util/difficulty.hpp"

namespace ses {
namespace proxy {

// For pool templates which can be broken down into several jobs, which
// can be broken down for nice hashing
class WorkerJobTemplate : public BaseJobTemplate
{
public:
  WorkerJobTemplate(const std::string& identifier, const std::string& jobIdentifier, const Algorithm& algorithm,
                    const Blob& blob, uint64_t difficulty, uint32_t height, uint32_t targetDifficulty)
    : BaseJobTemplate(identifier, jobIdentifier, algorithm, blob), nextClientNonce_(1), difficulty_(difficulty),
      height_(height), targetDifficulty_(targetDifficulty)
  {
  }

  void submitResult(const JobResult& result,
                    const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {
    //TODO implement
//    JobResult modifiedResult = result;
//    result.setWorkerNonce()
  }

  stratum::Job asStratumJob() const
  {
    stratum::Job stratumJob = blob_.asStratumJob();
    stratumJob.setId(identifier_);
    stratumJob.setJobIdentifier(jobIdentifier_);
    stratumJob.setDifficulty(std::to_string(difficulty_));
    stratumJob.setHeight(std::to_string(height_));
    return stratumJob;
  }

  void toStream(std::ostream& stream) const override
  {
    stream << "WorkerJobTemplate, jobId, " << jobIdentifier_;
  }

protected:
  Job::Ptr getNextSubJob(const WorkerIdentifier& workerIdentifier, WorkerType workerType) override
  {
    LOG_TRACE << __PRETTY_FUNCTION__;
    Job::Ptr job;
    if (workerType != WorkerType::PROXY)
    {
      //TODO limits checking
      Blob blob = blob_;
      blob.setClientNonce(nextClientNonce_);
      blob.convertToHashBlob();
      JobResult::Handler resultHandler =
        std::bind(&WorkerJobTemplate::handleResult,
                  std::dynamic_pointer_cast<WorkerJobTemplate>(shared_from_this()),
                  nextClientNonce_,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      job = Job::createMinerJob(workerIdentifier, generateJobIdentifier(), algorithm_, std::move(blob),
                                util::difficultyToTarget(targetDifficulty_), resultHandler);
      ++nextClientNonce_;
      //TODO connect ResultHandler
    }
    return job;
  }

private:
  void handleResult(uint32_t workerNonce,
                    const std::string& workerIdentifier,
                    const JobResult& jobResult,
                    const JobResult::SubmitStatusHandler& submitStatusHandler)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    LOG_TRACE << __PRETTY_FUNCTION__ << ", workerNonce, " << workerNonce;

    if (jobResult.getDifficulty() < targetDifficulty_)
    {
      submitStatusHandler(JobResult::SUBMIT_REJECTED_LOW_DIFFICULTY_SHARE);
    }
    else
    {
      JobResult modifiedResult = jobResult;
      modifiedResult.setIsNodeJsResult_(true);
      modifiedResult.setWorkerNonce(workerNonce);
      modifiedResult.setJobId(jobIdentifier_);
      //TODO add intermediate submitStatusHandler
      jobResultHandler_(identifier_, modifiedResult, submitStatusHandler);
    }
  }

private:
  uint32_t nextClientNonce_;
  uint64_t difficulty_;
  uint32_t height_;
  uint32_t targetDifficulty_;
};

} // namespace proxy
} // namespace ses