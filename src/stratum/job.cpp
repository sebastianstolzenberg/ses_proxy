#include <boost/algorithm/hex.hpp>

#include "stratum/job.hpp"

namespace ses {
namespace stratum {

namespace {
const size_t NONCE_OFFSET = 39;

std::vector<uint8_t> parseBlob(const std::string& blobHexString)
{
  std::vector<uint8_t> blob;
  boost::algorithm::unhex(blobHexString, std::back_inserter(blob));
  return blob;
}

uint64_t parseTarget(std::string targetHexString)
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

Job::Job(const std::string& blobHexString, const std::string& jobId, const std::string& targetHexString,
         const std::string& id)
  : blob_(parseBlob(blobHexString))
  , jobId_(jobId)
  , target_(parseTarget(targetHexString))
  , id_(id)
{
}

bool Job::isValid() const
{
  const size_t BLOB_SIZE_MAX = 84;
  const size_t BLOB_SIZE_MIN = 76;
  return target_ != 0 && blob_.size() >= BLOB_SIZE_MIN && blob_.size() <= BLOB_SIZE_MAX;
}

const std::vector<uint8_t>& Job::getBlob() const
{
  return blob_;
}

std::string Job::getBlobHexString() const
{
  std::string result;
  boost::algorithm::hex(blob_, std::back_inserter(result));
  return result;
}

const std::string& Job::getJobId() const
{
  return jobId_;
}

uint64_t Job::getTarget() const
{
  return target_;
}

std::string Job::getTargetHexString() const
{
  std::string result;
  boost::algorithm::hex(reinterpret_cast<const uint8_t*>(&target_),
                        reinterpret_cast<const uint8_t*>(&target_) + sizeof(target_),
                        std::back_inserter(result));
  return result;
}

const std::string& Job::getId() const
{
  return id_;
}

uint32_t Job::getNonce() const
{
  return *(reinterpret_cast<const uint32_t*>(blob_.data() + NONCE_OFFSET));
}

void Job::setNonce(uint32_t nonce)
{
  *(reinterpret_cast<uint32_t*>(blob_.data() + NONCE_OFFSET)) = nonce;
}

} // namespace stratum
} // namespace ses