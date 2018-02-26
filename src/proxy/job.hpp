#ifndef SES_PROXY_JOB_HPP
#define SES_PROXY_JOB_HPP

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <functional>

#include "stratum/job.hpp"
#include "proxy/workeridentifier.hpp"
#include "proxy/jobresult.hpp"

namespace ses {
namespace proxy {

class Job : public std::enable_shared_from_this<Job>
{
public:
  typedef std::shared_ptr<Job> Ptr;
  enum SubmitStatus
  {
    SUBMIT_ACCEPTED,
    SUBMIT_REJECTED_IP_BANNED,
    SUBMIT_REJECTED_UNAUTHENTICATED,
    SUBMIT_REJECTED_DUPLICATE,
    SUBMIT_REJECTED_EXPIRED,
    SUBMIT_REJECTED_INVALID_JOB_ID,
    SUBMIT_REJECTED_LOW_DIFFICULTY_SHARE
  };
  typedef std::function<void(SubmitStatus submitStatus)> SubmitStatusHandler;
  typedef std::function<void(const WorkerIdentifier& workerIdentifier,
                             const JobResult& jobResult,
                             const SubmitStatusHandler& submitStatusHandler)> JobResultHandler;

public:
  Job(const stratum::Job& stratumJob);

  void setJobResultHandler(const JobResultHandler& jobResultHandler);

  void invalidate();
  bool isValid() const;

  virtual void submitResult(const JobResult& result,
                            const SubmitStatusHandler& submitStatusHandler);

  stratum::Job asStratumJob() const;
  uint32_t getNonce() const;
  void setNonce(uint32_t nonce);
  uint8_t getNiceHash() const;
  void setNiceHash(uint8_t niceHash);

  const WorkerIdentifier& getAssignedWorker() const;
  void setAssignedWorker(const WorkerIdentifier& workerIdentifier);
  const std::string& getJobId() const;
  const std::vector<uint8_t>& getBlob() const;
  uint64_t getTarget() const;

protected:
  void submitResult(const WorkerIdentifier& workerIdentifier,
                    const JobResult& result,
                    const SubmitStatusHandler& submitStatusHandler);

protected:
  JobResultHandler jobResultHandler_;
  WorkerIdentifier assignedWorker_;
  std::string jobId_;
  std::vector<uint8_t> blob_;
  uint64_t target_;
};

class SubJob : public Job
{
public:
  typedef std::shared_ptr<SubJob> Ptr;

public:
  SubJob(const Job& masterJob)
    : Job(masterJob)
  {
  }

  void submitResult(const JobResult& result,
                    const SubmitStatusHandler& submitStatusHandler) override
  {
    if (foundNonces_.count(result.getNonce()) == 0)
    {
      submitStatusHandler(SUBMIT_REJECTED_DUPLICATE);
    }
    else if (result.getNiceHash() != getNiceHash())
    {
      // "Malformed nonce";
      submitStatusHandler(SUBMIT_REJECTED_DUPLICATE);
    }
    else if (!isValid() || !jobResultHandler_ )
    {
      submitStatusHandler(SUBMIT_REJECTED_INVALID_JOB_ID);
    }
    else
    {
      SubmitStatusHandler subJobSubmitStatusHandler = submitStatusHandler;
//          std::bind(&SubJob::handleSubmitStatus, shared_from_this(),
//                    std::placeholders::_1, submitStatusHandler, result.getNonce());
      jobResultHandler_(getAssignedWorker(), result, subJobSubmitStatusHandler);
    }
  }

private:
  void handleSubmitStatus(SubmitStatus submitStatus, SubmitStatusHandler handler, uint32_t nonce)
  {
    if (submitStatus == SUBMIT_ACCEPTED)
    {
      foundNonces_.insert(nonce);
    }
  }

private:
  std::set<uint32_t> foundNonces_;
};

class MasterJob : public Job
{
public:
  typedef std::shared_ptr<MasterJob> Ptr;

public:
  MasterJob(const stratum::Job& stratumJob)
    : Job(stratumJob), nextNiceHash_(0)
  {
  }

  Job::Ptr getSubJob(const WorkerIdentifier& workerIdentifier)
  {
    SubJob::Ptr subJob;
    auto subJobIt = subJobs_.find(workerIdentifier);
    if (subJobIt == subJobs_.end())
    {
      // new worker
      subJob = getNextSubJob();
    }
    else
    {
      subJob = subJobIt->second;
    }
    return subJob;
  }

private:
  SubJob::Ptr getNextSubJob()
  {
    SubJob::Ptr subJob;
    // If the masterjob has been nice-hashed by the upstream proxy or pool already,
    // further breakdown is not possible and only one subjob can be returned.
    const uint8_t maxNiceHash = (getNiceHash() == 0) ? std::numeric_limits<uint8_t>::max() : 0;
    if (nextNiceHash_ <= maxNiceHash)
    {
      subJob = std::make_shared<SubJob>(*this);
      subJob->setNiceHash(nextNiceHash_++);
      JobResultHandler modifyingJobResultHandler =
          std::bind(&MasterJob::modifyAndSubmitResult, shared_from_this(),
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      subJob->setJobResultHandler(modifyingJobResultHandler);
    }
    return subJob;
  }

  void modifyAndSubmitResult(const WorkerIdentifier& workerIdentifier,
                             const JobResult& result,
                             const SubmitStatusHandler& submitStatusHandler)
  {
    JobResult modifiedResult = result;
    modifiedResult.setJobId(getJobId());
    submitResult(workerIdentifier, modifiedResult, submitStatusHandler);
  }

private:
  std::map<WorkerIdentifier, SubJob::Ptr> subJobs_;
  uint32_t nextNiceHash_;
};

} // namespace proxy
} // namespace ses

#endif //SES_PROXY_JOB_HPP