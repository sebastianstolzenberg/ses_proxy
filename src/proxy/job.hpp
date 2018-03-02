#ifndef SES_PROXY_JOB_HPP
#define SES_PROXY_JOB_HPP

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

namespace ses {
namespace proxy {

class Job : public std::enable_shared_from_this<Job>
{
public:
  typedef std::shared_ptr<Job> Ptr;

public:
  Job(const stratum::Job& stratumJob)
  {
    //TODO generate random job id
  }

  virtual ~Job() = default;

  void setJobResultHandler(const JobResult::Handler& jobResultHandler);

  bool isValid() const;

  virtual void submitResult(const JobResult& result,
                            const JobResult::SubmitStatusHandler& submitStatusHandler);

  size_t numHashesFound() const;

  virtual stratum::Job asStratumJob() const = 0;

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
  Job() = default;
  void submitResult(const WorkerIdentifier& workerIdentifier,
                    const JobResult& result,
                    const JobResult::SubmitStatusHandler& submitStatusHandler);

protected:
  JobResult::Handler jobResultHandler_;
  WorkerIdentifier assignedWorker_;
  std::string jobId_;
  std::vector<uint8_t> blob_;
  uint64_t target_;
};

class MinerJob : public Job
{
public:
  MinerJob(const std::vector<uint8_t>& blob, uint64_t target)
    : blob_(blob), target_(target)
  {

  }

  stratum::Job asStratumJob() const override;

private:
  std::vector<uint8_t> blob_;
  uint64_t target_;
};

class ProxyJob : public Job
{
public:
  ProxyJob(const std::vector<uint8_t>& blobTemplate, uint64_t difficulty, uint32_t height,
           uint32_t reservedOffset, uint32_t clientNonceOffset, uint32_t clientPoolOffset,
           uint64_t targetDiff)
    : blobTemplate_(blobTemplate) , difficulty_(difficulty), height_(height),
      reservedOffset_(reservedOffset), clientNonceOffset_(clientNonceOffset),
      clientPoolOffset_(clientPoolOffset), targetDiff_(targetDiff)
  {

  }

  stratum::Job asStratumJob() const override;

private:
  std::vector<uint8_t> blobTemplate_;
  uint64_t difficulty_;
  uint32_t height_;
  uint32_t reservedOffset_;
  uint32_t clientNonceOffset_;
  uint32_t clientPoolOffset_;
  uint64_t targetDiff_;
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
                    const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {
    if (foundNonces_.count(result.getNonce()) > 0)
    {
      submitStatusHandler(JobResult::SUBMIT_REJECTED_DUPLICATE);
    }
    else if (getNiceHash() != 0 && result.getNiceHash() != getNiceHash())
    {
      // "Malformed nonce";
      submitStatusHandler(JobResult::SUBMIT_REJECTED_DUPLICATE);
    }
    else if (!isValid() || !jobResultHandler_ )
    {
      submitStatusHandler(JobResult::SUBMIT_REJECTED_INVALID_JOB_ID);
    }
    else
    {
      JobResult::SubmitStatusHandler subJobSubmitStatusHandler =
          std::bind(&SubJob::handleSubmitStatus, std::dynamic_pointer_cast<SubJob>(shared_from_this()),
                    std::placeholders::_1, submitStatusHandler, result.getNonce());
      jobResultHandler_(getAssignedWorker(), result, subJobSubmitStatusHandler);
    }
  }

  size_t numHashesFound() const override
  {
    return foundNonces_.size();
  }

private:
  void handleSubmitStatus(JobResult::SubmitStatus submitStatus, JobResult::SubmitStatusHandler handler, uint32_t nonce)
  {
    std::cout << __PRETTY_FUNCTION__ << " " << submitStatus << std::endl;
    if (submitStatus == JobResult::SUBMIT_ACCEPTED)
    {
      foundNonces_.insert(nonce);
    }
    handler(submitStatus);
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
      subJob = getNextSubJob(workerIdentifier);
    }
    else
    {
      subJob = subJobIt->second;
    }
    return subJob;
  }

  size_t numHashesFound() const override
  {
    return std::accumulate(subJobs_.begin(), subJobs_.end(), 0,
                           [](size_t sum, const auto& subJob) -> size_t
                           {
                             return sum + (subJob.second ? subJob.second->numHashesFound() : 0);
                           });
  }

private:
  SubJob::Ptr getNextSubJob(const WorkerIdentifier& workerIdentifier)
  {
    SubJob::Ptr subJob;
    // If the masterjob has been nice-hashed by the upstream proxy or pool already,
    // further breakdown is not possible and only one subjob can be returned.
    const uint8_t maxNiceHash = (getNiceHash() == 0) ? std::numeric_limits<uint8_t>::max() : 0;
    if (nextNiceHash_ <= maxNiceHash)
    {
      subJob = std::make_shared<SubJob>(*this);
      subJob->setNiceHash(nextNiceHash_++);
      JobResult::Handler modifyingJobResultHandler =
        std::bind(&MasterJob::modifyAndSubmitResult, std::dynamic_pointer_cast<MasterJob>(shared_from_this()),
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      subJob->setAssignedWorker(workerIdentifier);
      subJob->setJobResultHandler(modifyingJobResultHandler);
      subJobs_[workerIdentifier] = subJob;
    }
    return subJob;
  }

  void modifyAndSubmitResult(const WorkerIdentifier& workerIdentifier,
                             const JobResult& result,
                             const JobResult::SubmitStatusHandler& submitStatusHandler)
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
