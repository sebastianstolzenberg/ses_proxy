#include "stratum/job.hpp"

namespace ses {
namespace stratum {

Job::Job(const std::string& id, const std::string& jobId, const std::string& blobHexString,
         const std::string& targetHexString)
  : id_(id)
  , jobId_(jobId)
  , blob_(blobHexString)
  , target_(targetHexString)
{
}

Job::Job(const std::string& id, const std::string& jobId, const std::string& blobHexString,
         const std::string& targetHexString,
         const std::string& blocktemplate_blob, const std::string& difficulty, const std::string& height,
         const std::string& reserved_offset, const std::string& client_nonce_offset,
         const std::string& client_pool_offset, const std::string& target_diff, const std::string& target_diff_hex,
         const std::string& job_id)
  : id_(id)
  , jobId_(job_id.empty() ? jobId : job_id)
  , blob_(blobHexString)
  , target_(targetHexString)
  , blocktemplateBlob_(blocktemplate_blob)
  , difficulty_(difficulty)
  , height_(height)
  , reservedOffset_(reserved_offset)
  , clientNonceOffset_(client_nonce_offset)
  , clientPoolOffset_(client_pool_offset)
  , targetDiff_(target_diff)
  , targetDiffHex_(target_diff_hex)
{
}

const std::string& Job::getId() const
{
  return id_;
}

const std::string& Job::getJobId() const
{
  return jobId_;
}

const std::string& Job::getBlob() const
{
  return blob_;
}

const std::string& Job::getTarget() const
{
  return target_;
}

const std::string& Job::getBlocktemplateBlob() const
{
  return blocktemplateBlob_;
}

const std::string& Job::getDifficulty() const
{
  return difficulty_;
}

const std::string& Job::getHeight() const
{
  return height_;
}

const std::string& Job::getReservedOffset() const
{
  return reservedOffset_;
}

const std::string& Job::getClientNonceOffset() const
{
  return clientNonceOffset_;
}

const std::string& Job::getClientPoolOffset() const
{
  return clientPoolOffset_;
}

const std::string& Job::getTargetDiff() const
{
  return targetDiff_;
}

const std::string& Job::getTargetDiffHex() const
{
  return targetDiffHex_;
}

bool Job::isBlockTemplate() const
{
  return !blocktemplateBlob_.empty();
}

} // namespace stratum
} // namespace ses