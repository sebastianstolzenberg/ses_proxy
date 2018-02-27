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
  , jobId_(jobId)
  , blob_(blobHexString)
  , target_(targetHexString)
  , blocktemplate_blob_(blocktemplate_blob)
  , difficulty_(difficulty)
  , height_(height)
  , reserved_offset_(reserved_offset)
  , client_nonce_offset_(client_nonce_offset)
  , client_pool_offset_(client_pool_offset)
  , target_diff_(target_diff)
  , target_diff_hex_(target_diff_hex)
  , job_id_(job_id)
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

const std::string& Job::getBlocktemplate_blob() const
{
  return blocktemplate_blob_;
}

const std::string& Job::getDifficulty() const
{
  return difficulty_;
}

const std::string& Job::getHeight() const
{
  return height_;
}

const std::string& Job::getReserved_offset() const
{
  return reserved_offset_;
}

const std::string& Job::getClient_nonce_offset() const
{
  return client_nonce_offset_;
}

const std::string& Job::getClient_pool_offset() const
{
  return client_pool_offset_;
}

const std::string& Job::getTarget_diff() const
{
  return target_diff_;
}

const std::string& Job::getTarget_diff_hex() const
{
  return target_diff_hex_;
}

const std::string& Job::getJob_id() const
{
  return job_id_;
}

bool Job::isNodeJs() const
{
  return !blocktemplate_blob_.empty();
}

} // namespace stratum
} // namespace ses