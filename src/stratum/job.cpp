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

void Job::setId(const std::string& id)
{
  id_ = id;
}

const std::string& Job::getJobIdentifier() const
{
  return jobId_;
}

void Job::setJobIdentifier(const std::string& jobId)
{
  jobId_ = jobId;
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

void Job::setDifficulty(const std::string& difficulty)
{
  difficulty_ = difficulty;
}

const std::string& Job::getHeight() const
{
  return height_;
}

void Job::setHeight(const std::string& height)
{
  height_ = height;
}

const std::string& Job::getReservedOffset() const
{
  return reservedOffset_;
}

void Job::setReservedOffset(const std::string& reservedOffset)
{
  reservedOffset_ = reservedOffset;
}

const std::string& Job::getClientNonceOffset() const
{
  return clientNonceOffset_;
}

void Job::setClientNonceOffset(const std::string& clientNonceOffset)
{
  clientNonceOffset_ = clientNonceOffset;
}

const std::string& Job::getClientPoolOffset() const
{
  return clientPoolOffset_;
}

void Job::setClientPoolOffset(const std::string& clientPoolOffset)
{
  clientPoolOffset_ = clientPoolOffset;
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