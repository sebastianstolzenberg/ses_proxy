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

} // namespace stratum
} // namespace ses