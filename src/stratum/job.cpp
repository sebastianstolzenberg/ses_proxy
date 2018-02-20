#include <boost/algorithm/hex.hpp>

#include "stratum/job.hpp"

namespace ses {
namespace stratum {

Job::Job(const std::string& blob, const std::string& jobId, const std::string& target, const std::string& id)
  : jobId_(jobId)
  , id_(id)
{
  boost::algorithm::unhex(blob, std::back_inserter(blob_));
}

const std::vector& Job::getBlob() const
{
  return blob_;
}

const std::string& Job::getJobId() const
{
  return jobId_;
}

uint64_t Job::getTarget() const
{
  return target_;
}

const std::string& Job::getId() const
{
  return id_;
}
} // namespace stratum
} // namespace ses