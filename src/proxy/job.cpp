#include <boost/algorithm/hex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "proxy/job.hpp"
#include "util/log.hpp"

namespace ses {
namespace proxy {

namespace {
std::string toHexString(uint32_t target)
{
  std::string targetHex;
//  if (target <= std::numeric_limits<uint32_t>::max())
//  {
//    boost::algorithm::hex_lower(reinterpret_cast<const uint8_t*>(&target),
//                                reinterpret_cast<const uint8_t*>(&target) + sizeof(target) / 2,
//                                std::back_inserter(targetHex));
//  }
//  else
  {
    boost::algorithm::hex_lower(reinterpret_cast<const uint8_t*>(&target),
                                reinterpret_cast<const uint8_t*>(&target) + sizeof(target),
                                std::back_inserter(targetHex));
  }
  return targetHex;
}
}

class MinerJob: public Job, public std::enable_shared_from_this<MinerJob>
{
public:
  MinerJob(const WorkerIdentifier& workerIdentifier, const std::string& jobIdentifier,
           const Blob& blob, uint32_t target, const JobResult::Handler& jobResultHandler)
    : assignedWorker_(workerIdentifier), jobIdentifier_(jobIdentifier), blob_(blob), target_(target),
      jobResultHandler_(jobResultHandler)
  {
  }

  void submitResult(const JobResult& result,
                    const JobResult::SubmitStatusHandler& submitStatusHandler) override
  {
    if (jobResultHandler_ && result.getJobIdentifier() == jobIdentifier_)
    {
      jobResultHandler_(assignedWorker_, result, submitStatusHandler);
    }
    else
    {
      if (submitStatusHandler)
      {
        submitStatusHandler(JobResult::SUBMIT_REJECTED_INVALID_JOB_ID);
      }
    }
  }

  size_t numHashesFound() const
  {
    return 0;
  }

  size_t currentHashRate() const
  {
    return 0;
  }

  const std::string& getJobIdentifier() const
  {
    return jobIdentifier_;
  }

  stratum::Job asStratumJob() const
  {
    return stratum::Job(toString(assignedWorker_), jobIdentifier_, blob_.toHexString(), toHexString(target_));
  }

private:
  JobResult::Handler jobResultHandler_;
  WorkerIdentifier assignedWorker_;
  std::string jobIdentifier_;
  Blob blob_;
  uint32_t target_;
};


Job::Ptr Job::createMinerJob(const WorkerIdentifier& workerIdentifier, const std::string& jobIdentifier,
                        const Blob& blob, uint32_t target, const JobResult::Handler& jobResultHandler)
{
  return std::make_shared<MinerJob>(workerIdentifier, jobIdentifier, blob, target, jobResultHandler);
}

} // namespace proxy
} // namespace ses