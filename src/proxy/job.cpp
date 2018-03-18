#include <boost/algorithm/hex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "proxy/job.hpp"
#include "util/difficulty.hpp"

namespace ses {
namespace proxy {

class MinerJob: public Job, public std::enable_shared_from_this<MinerJob>
{
public:
  MinerJob(const WorkerIdentifier& workerIdentifier, const std::string& jobIdentifier,
           const Blob& blob, const util::Target& target, const JobResult::Handler& jobResultHandler)
    : assignedWorker_(workerIdentifier), jobIdentifier_(jobIdentifier), blob_(blob), target_(target),
      jobResultHandler_(jobResultHandler)
  {
  }

  void submitResult(const JobResult& result,
                    const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {
    if (jobResultHandler_ && result.getJobIdentifier() == jobIdentifier_)
    {
      jobResultHandler_(toString(assignedWorker_), result, submitStatusHandler);
    }
    else
    {
      if (submitStatusHandler)
      {
        submitStatusHandler(JobResult::SUBMIT_REJECTED_INVALID_JOB_ID);
      }
    }
  }

  size_t numHashesFound() const override
  {
    return 0;
  }

  size_t currentHashRate() const override
  {
    return 0;
  }

  const std::string& getJobIdentifier() const override
  {
    return jobIdentifier_;
  }

  util::Target getTarget() const override
  {
    return target_;
  }

  uint32_t getDifficulty() const override
  {
    return util::targetToDifficulty(getTarget());
  }

  stratum::Job asStratumJob() const override
  {
    return stratum::Job(toString(assignedWorker_), jobIdentifier_, blob_.toHexString(), target_.toHexString());
  }

private:
  JobResult::Handler jobResultHandler_;
  WorkerIdentifier assignedWorker_;
  std::string jobIdentifier_;
  Blob blob_;
  util::Target target_;
};


Job::Ptr Job::createMinerJob(const WorkerIdentifier& workerIdentifier, const std::string& jobIdentifier,
                        const Blob& blob, const util::Target& target, const JobResult::Handler& jobResultHandler)
{
  return std::make_shared<MinerJob>(workerIdentifier, jobIdentifier, blob, target, jobResultHandler);
}

} // namespace proxy
} // namespace ses