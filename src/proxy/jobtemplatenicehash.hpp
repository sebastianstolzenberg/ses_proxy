#pragma once

#include "proxy/jobtemplatebase.hpp"
#include "util/target.hpp"

namespace ses {
namespace proxy {

// For pool jobs which can only be broken down by nice hash separation
// i.e by presetting the highest byte of the nonce at offset 39 for each miner
class NiceHashJobTemplate : public BaseJobTemplate
{
public:
  NiceHashJobTemplate(const std::string& identifier, const std::string& jobIdentifier, const Algorithm& algorithm,
                      const Blob& blob, const util::Target target)
    : BaseJobTemplate(identifier, jobIdentifier, algorithm, blob), target_(target),
      targetDifficulty_(util::targetToDifficulty(target)), nextNiceHash_(0)
  {
  }

  void submitResult(const JobResult& result,
                    const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {
    LOG_TRACE << __PRETTY_FUNCTION__;
  }

  uint32_t getDifficulty() const override
  {
    return targetDifficulty_;
  }

  void toStream(std::ostream& stream) const override
  {
    stream << "NiceHashJobTemplate, jobId, " << jobIdentifier_
           << ", target, " << target_.toHexString()
           << ", difficulty, " << targetDifficulty_;
  }

private:
  Job::Ptr getNextSubJob(const WorkerIdentifier& workerIdentifier, WorkerType workerType) override
  {
    LOG_TRACE << __PRETTY_FUNCTION__ << " " << typeid(this).name();
    Job::Ptr job;
    if (workerType != WorkerType::PROXY)
    {
      if (nextNiceHash_ <= std::numeric_limits<uint8_t>::max())
      {
        Blob blob = blob_;
        blob.setNiceHash(nextNiceHash_);
        ++nextNiceHash_;
        JobResult::Handler resultHandler =
          std::bind(&NiceHashJobTemplate::handleResult,
                    std::dynamic_pointer_cast<NiceHashJobTemplate>(shared_from_this()),
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        job = Job::createMinerJob(workerIdentifier, generateJobIdentifier(), algorithm_, std::move(blob), target_,
                                  resultHandler);
//        LOG_DEBUG << "NiceHashJobTemplate::getNextSubJob generated new blob with start nonce " << blob.getNonce();
        //TODO connect ResultHandler
      }
    }
    return job;
  }

  void handleResult(const std::string& workerIdentifier,
                    const JobResult& jobResult,
                    const JobResult::SubmitStatusHandler& submitStatusHandler)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
//    LOG_DEBUG << __PRETTY_FUNCTION__;

    if (foundNonces_.count(jobResult.getNonce()) > 0)
    {
      submitStatusHandler(JobResult::SUBMIT_REJECTED_DUPLICATE);
    }
    else if (!jobResultHandler_ )
    {
      submitStatusHandler(JobResult::SUBMIT_REJECTED_INVALID_JOB_ID);
    }
    else
    {
      JobResult modifiedResult = jobResult;
      modifiedResult .setJobId(jobIdentifier_);
      JobResult::SubmitStatusHandler subJobSubmitStatusHandler =
          std::bind(&NiceHashJobTemplate::handleSubmitStatus,
                    std::dynamic_pointer_cast<NiceHashJobTemplate>(shared_from_this()),
                    std::placeholders::_1, submitStatusHandler, modifiedResult.getNonce());
      jobResultHandler_(identifier_, modifiedResult, subJobSubmitStatusHandler);
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
  util::Target target_;
  uint32_t targetDifficulty_;
  uint32_t nextNiceHash_;
  std::set<uint32_t> foundNonces_;
};

} // namespace proxy
} // namespace ses