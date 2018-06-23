#pragma once

#include "proxy/jobtemplateworker.hpp"

namespace ses {
namespace proxy {

// For pool templates which can be broken down into several WorkerJobTemplates
//TODO solo mode as in xmr-node-proxy
class MasterJobTemplate : public BaseJobTemplate
{
public:
  MasterJobTemplate(const std::string& identifier, const std::string& jobIdentifier, const Algorithm& algorithm,
                    const Blob& blob, uint64_t difficulty, uint32_t height, uint32_t targetDifficulty)
    : BaseJobTemplate(identifier, jobIdentifier, algorithm, blob), nextPoolNonce_(1), difficulty_(difficulty),
      height_(height), targetDifficulty_(targetDifficulty)
  {
  }

  void submitResult(const JobResult& result,
                    const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {

  }

  uint32_t getDifficulty() const override
  {
    return targetDifficulty_;
  }

  bool supportsWorkerType(WorkerType workerType) override
  {
    return true;
  }

  void toStream(std::ostream& stream) const override
  {
    stream << "MasterJobTemplate, jobId, " << jobIdentifier_
           << ", difficulty, " << difficulty_
           << ", height, " << height_
           << ", targetDifficulty, " << targetDifficulty_
           << ", blob, " << blob_.toHexString();
  }

protected:
  Job::Ptr getNextSubJob(const WorkerIdentifier& workerIdentifier, WorkerType workerType) override
  {
    Job::Ptr job;
    if (workerType == WorkerType::PROXY)
    {
      JobTemplate::Ptr jobTemplate = getNextSubjacentTemplate(workerIdentifier);
      //TODO connect ResultHandler
      job = jobTemplate;
    }
    else
    {
      if (!activeSubJobTemplate_ || !(job = activeSubJobTemplate_->getJobFor(workerIdentifier, workerType)))
      {
        activeSubJobTemplate_ = getNextSubjacentTemplate(generateWorkerIdentifier());
        //TODO connect ResultHandler
        if (activeSubJobTemplate_)
        {
          job = activeSubJobTemplate_->getJobFor(workerIdentifier, workerType);
          //TODO connect ResultHandler
        }
      }
    }
    return job;
  }

  JobTemplate::Ptr getSubjacentTemplateFor(const WorkerIdentifier& workerIdentifier)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    JobTemplate::Ptr subTemplate;
    auto subTemplateIt = subTemplates_.find(workerIdentifier);
    if (subTemplateIt == subTemplates_.end())
    {
      // new worker
      subTemplate = getNextSubjacentTemplate(workerIdentifier);
      subTemplates_[workerIdentifier] = subTemplate;
    }
    else
    {
      subTemplate = subTemplateIt->second;
    }
    return subTemplate;
  }


  JobTemplate::Ptr getNextSubjacentTemplate(const WorkerIdentifier& workerIdentifier)
  {
    JobTemplate::Ptr subTemplate;
    //TODO limits checking
    Blob blob = blob_;
    blob.setClientPool(nextPoolNonce_);
    subTemplate =
      std::make_shared<WorkerJobTemplate>(toString(workerIdentifier), generateJobIdentifier(), algorithm_,
                                          std::move(blob), difficulty_, height_, targetDifficulty_);
    subTemplate->setJobResultHandler(
      std::bind(&MasterJobTemplate::handleResult,
                std::dynamic_pointer_cast<MasterJobTemplate>(shared_from_this()),
                nextPoolNonce_,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    ++nextPoolNonce_;
    //TODO connect ResultHandler
    return subTemplate;
  }

private:
  void handleResult(uint32_t poolNonce,
                    const std::string& workerIdentifier,
                    const JobResult& jobResult,
                    const JobResult::SubmitStatusHandler& submitStatusHandler)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
//    LOG_DEBUG << __PRETTY_FUNCTION__ << ", poolNonce, " << poolNonce;

    JobResult modifiedResult = jobResult;
    modifiedResult.setIsNodeJsResult_(true);
    modifiedResult.setPoolNonce(poolNonce);
    modifiedResult.setJobId(jobIdentifier_);
    //TODO add intermediate submitStatusHandler
    jobResultHandler_(identifier_, modifiedResult, submitStatusHandler);
  }

private:
  std::map<WorkerIdentifier, WorkerJobTemplate::Ptr> subTemplates_;
  uint32_t nextPoolNonce_;
  WorkerJobTemplate::Ptr activeSubJobTemplate_;
  uint64_t difficulty_;
  uint32_t height_;
  uint32_t targetDifficulty_;
};

} // namespace proxy
} // namespace ses