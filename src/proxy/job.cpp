#include <boost/algorithm/hex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "proxy/job.hpp"

namespace ses {
namespace proxy {

namespace {
const size_t NONCE_OFFSET = 39;

std::vector<uint8_t> parseBlob(const std::string& blobHexString)
{
  std::vector<uint8_t> blob;
  boost::algorithm::unhex(blobHexString, std::back_inserter(blob));
  return blob;
}

uint64_t parseTarget(const std::string& targetHexString)
{
  uint64_t target = 0;
  if (targetHexString.size() <= 2 * sizeof(uint64_t))
  {
    boost::algorithm::unhex(targetHexString, reinterpret_cast<uint8_t*>(&target));
    if (targetHexString.size() <= 2 * sizeof(uint32_t))
    {
      // multiplication necessary
      target = (0xFFFFFFFFFFFFFFFFULL / 0xFFFFFFFFULL) * target;
    }
  }
  return target;
}
}

Job::Job(const stratum::Job& stratumJob)
  : assignedWorker_(boost::lexical_cast<WorkerIdentifier>(stratumJob.getId()))
  , jobId_(stratumJob.getJobId())
  , blob_(parseBlob(stratumJob.getBlob()))
  , target_(parseTarget(stratumJob.getTarget()))
{
};

stratum::Job Job::asStratumJob() const
{
  std::string blobHex;
  boost::algorithm::hex_lower(blob_, std::back_inserter(blobHex));

  std::string targetHex;
  boost::algorithm::hex_lower(reinterpret_cast<const uint8_t*>(&target_),
                        reinterpret_cast<const uint8_t*>(&target_) + sizeof(target_),
                        std::back_inserter(targetHex));

  return stratum::Job(boost::uuids::to_string(assignedWorker_), jobId_, blobHex, targetHex);
}

void Job::setJobResultHandler(const Job::JobResultHandler& jobResultHandler)
{
  jobResultHandler_ = jobResultHandler;
}

void Job::invalidate()
{
  blob_.clear();
  target_ = 0;
}

bool Job::isValid() const
{
  const size_t BLOB_SIZE_MAX = 84;
  const size_t BLOB_SIZE_MIN = 76;
  return target_ != 0 && blob_.size() >= BLOB_SIZE_MIN && blob_.size() <= BLOB_SIZE_MAX;
}

void Job::submitResult(const JobResult& result,
                       const SubmitStatusHandler& submitStatusHandler)
{
  submitResult(assignedWorker_, result, submitStatusHandler);
}

uint32_t Job::getNonce() const
{
  return *(reinterpret_cast<const uint32_t*>(blob_.data() + NONCE_OFFSET));
}

void Job::setNonce(uint32_t nonce)
{
  *(reinterpret_cast<uint32_t*>(blob_.data() + NONCE_OFFSET)) = nonce;
}

uint8_t Job::getNiceHash() const
{
  return *(reinterpret_cast<const uint8_t*>(blob_.data() + NONCE_OFFSET));
}

void Job::setNiceHash(uint8_t niceHash)
{
  *(reinterpret_cast<uint8_t*>(blob_.data() + NONCE_OFFSET)) = niceHash;
}

const WorkerIdentifier& Job::getAssignedWorker() const
{
  return assignedWorker_;
}

void Job::setAssignedWorker(const WorkerIdentifier& workerIdentifier)
{
  assignedWorker_ = workerIdentifier;
}

const std::string& Job::getJobId() const
{
  return jobId_;
}

const std::vector<uint8_t>& Job::getBlob() const
{
  return blob_;
}

uint64_t Job::getTarget() const
{
  return target_;
}

void Job::submitResult(const WorkerIdentifier& workerIdentifier,
                       const JobResult& result,
                       const SubmitStatusHandler& submitStatusHandler)
{
  if (!jobResultHandler_)
  {
    if (submitStatusHandler)
    {
      submitStatusHandler(SUBMIT_REJECTED_INVALID_JOB_ID);
    }
  }
  else
  {
    jobResultHandler_(workerIdentifier, result, submitStatusHandler);
  }
}

} // namespace proxy
} // namespace ses