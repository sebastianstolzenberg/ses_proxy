#include <mutex>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/random_generator.hpp>

#include "proxy/jobtemplate.hpp"
#include "proxy/blob.hpp"
#include "proxy/workeridentifier.hpp"
#include "util/difficulty.hpp"
#include "util/log.hpp"

namespace ses {
namespace proxy {

namespace {
const size_t NONCE_OFFSET = 39;

uint64_t parseTarget(const std::string& targetHexString)
{
  uint64_t target = 0;
  if (targetHexString.size() <= 2 * sizeof(uint64_t))
  {
    boost::algorithm::unhex(targetHexString, reinterpret_cast<uint8_t*>(&target));
    if (targetHexString.size() <= 2 * sizeof(uint32_t))
    {
      // multiplication necessary
      uint32_t halfTarget = 0;
      boost::algorithm::unhex(targetHexString, reinterpret_cast<uint8_t*>(&halfTarget));
      target = halfTarget;
      target = 0xFFFFFFFFFFFFFFFFULL / (0xFFFFFFFFULL / target);
    }
    else
    {
      boost::algorithm::unhex(targetHexString, reinterpret_cast<uint8_t*>(&target));
    }
  }
  return target;
}

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

// For pool templates which can be broken down into several jobs, which
// can be broken down for nice hashing
class WorkerJobTemplate : public BaseJobTemplate
{
public:
  WorkerJobTemplate(const WorkerIdentifier& identifier, const std::string& jobIdentifier, const Blob& blob,
                    uint64_t difficulty, uint32_t height, uint32_t targetDiff)
    : BaseJobTemplate(identifier, jobIdentifier, blob), nextClientNonce_(1), difficulty_(difficulty),
      height_(height), targetDiff_(targetDiff)
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
    stratumJob.setId(toString(identifier_));
    stratumJob.setJobIdentifier(jobIdentifier_);
    stratumJob.setDifficulty(std::to_string(difficulty_));
    stratumJob.setHeight(std::to_string(height_));
    return stratumJob;
  }

protected:
  Job::Ptr getNextSubJob(const WorkerIdentifier& workerIdentifier, WorkerType workerType) override
  {
    LOG_DEBUG << __PRETTY_FUNCTION__;
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
      job = Job::createMinerJob(workerIdentifier, generateJobIdentifier(), std::move(blob),
                                util::difficultyToTarget(targetDiff_), resultHandler);
      ++nextClientNonce_;
      //TODO connect ResultHandler
    }
    return job;
  }

private:
  void handleResult(uint32_t workerNonce,
                    const WorkerIdentifier& workerIdentifier,
                    const JobResult& jobResult,
                    const JobResult::SubmitStatusHandler& submitStatusHandler)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
//    LOG_DEBUG << __PRETTY_FUNCTION__ << ", workerNonce, " << workerNonce;

    if (jobResult.getDifficulty() < targetDiff_)
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
  uint32_t targetDiff_;
};

// For pool templates which can be broken down into several WorkerJobTemplates
//TODO solo mode as in xmr-node-proxy
class MasterJobTemplate : public BaseJobTemplate
{
public:
  MasterJobTemplate(const WorkerIdentifier& identifier, const std::string& jobIdentifier, const Blob& blob,
                    uint64_t difficulty, uint32_t height, uint32_t targetDiff)
    : BaseJobTemplate(identifier, jobIdentifier, blob), nextPoolNonce_(1), difficulty_(difficulty),
      height_(height), targetDiff_(targetDiff)
  {
  }

  void submitResult(const JobResult& result,
                    const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {

  }

  bool supportsWorkerType(WorkerType workerType) override
  {
    return true;
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
        activeSubJobTemplate_ = getNextSubjacentTemplate(identifier_);
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
      std::make_shared<WorkerJobTemplate>(workerIdentifier, generateJobIdentifier(), std::move(blob),
                                          difficulty_, height_, targetDiff_);
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
                    const WorkerIdentifier& workerIdentifier,
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
  uint32_t targetDiff_;
};

// For pool jobs which can only be broken down by nice hash separation
// i.e by presetting the highest byte of the nonce at offset 39 for each miner
class NiceHashJobTemplate : public BaseJobTemplate
{
public:
  NiceHashJobTemplate(const WorkerIdentifier& identifier, const std::string& jobIdentifier,
                      const Blob& blob, uint64_t target)
    : BaseJobTemplate(identifier, jobIdentifier, blob), target_(target), lastNiceHash_(0)
  {
  }

  void submitResult(const JobResult& result,
                    const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {
    LOG_DEBUG << __PRETTY_FUNCTION__;
  }

private:
  Job::Ptr getNextSubJob(const WorkerIdentifier& workerIdentifier, WorkerType workerType) override
  {
    LOG_DEBUG << __PRETTY_FUNCTION__ << " " << typeid(this).name();
    Job::Ptr job;
    if (workerType != WorkerType::PROXY)
    {
      if (lastNiceHash_ < std::numeric_limits<uint8_t>::max())
      {
        ++lastNiceHash_;
        Blob blob = blob_;
        blob.setNiceHash(lastNiceHash_);
        JobResult::Handler resultHandler =
          std::bind(&NiceHashJobTemplate::handleResult,
                    std::dynamic_pointer_cast<NiceHashJobTemplate>(shared_from_this()),
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        job = Job::createMinerJob(workerIdentifier, generateJobIdentifier(), std::move(blob), target_, resultHandler);
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
    LOG_DEBUG << __PRETTY_FUNCTION__ << " " << submitStatus << " " << nonce;
    if (submitStatus == JobResult::SUBMIT_ACCEPTED)
    {
      foundNonces_.insert(nonce);
    }
    handler(submitStatus);
  }

private:
  uint64_t target_;
  uint8_t lastNiceHash_;
  std::set<uint32_t> foundNonces_;
};

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
    LOG_DEBUG << __PRETTY_FUNCTION__ << " " << submitStatus << " " << nonce;
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

JobTemplate::Ptr JobTemplate::create(const stratum::Job& stratumJob)
{
  JobTemplate::Ptr jobTemplate;

  Blob blob(stratumJob);
  WorkerIdentifier identifier = toWorkerIdentifier(stratumJob.getId());
  std::string jobIdentifier = stratumJob.getJobIdentifier();

  if (blob.isTemplate())
  {
    // JobTemplate case
    uint32_t difficulty = boost::lexical_cast<uint64_t>(stratumJob.getDifficulty());
    uint32_t height = boost::lexical_cast<uint32_t>(stratumJob.getHeight());
    uint32_t targetDiff = boost::lexical_cast<uint32_t>(stratumJob.getTargetDiff());
    if (blob.hasClientPoolOffset())
    {
      jobTemplate =
          std::make_shared<MasterJobTemplate>(identifier, jobIdentifier, std::move(blob),
                                              difficulty, height, targetDiff);
    }
    else
    {
    }

  }
  else
  {
    // Job case
    uint64_t target = parseTarget(stratumJob.getTarget());
    if (blob.getNiceHash() == static_cast<uint8_t>(0))
    {
      jobTemplate = std::make_shared<NiceHashJobTemplate>(identifier, jobIdentifier, std::move(blob), target);
    }
    else
    {
      jobTemplate = std::make_shared<SoloJobTemplate>(identifier, jobIdentifier, std::move(blob), target);
    }
  }

  return jobTemplate;
}

} // namespace proxy
} // namespace ses