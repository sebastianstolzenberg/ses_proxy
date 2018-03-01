#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>

#include "proxy/jobtemplate.hpp"

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

JobTemplate::Ptr JobTemplate::create(const stratum::Job& stratumJob)
{
  JobTemplate::Ptr jobTemplate;
  return jobTemplate;
}

void JobTemplate::setJobResultHandler(const JobResult::Handler& jobResultHandler)
{
  jobResultHandler_ = jobResultHandler;
}

bool JobTemplate::canCreateSubjacentTemplates() const
{
  return false;
}

JobTemplate::Ptr JobTemplate::getSubjacentTemplateFor(const WorkerIdentifier&)
{
  // not supported, returns an null pointer
  return JobTemplate::Ptr();
}

// For pool templates which can be broken down into several jobs, which
// can be broken down for nice hashing
class WorkerJobTemplate : public JobTemplate
{
public:
  WorkerJobTemplate(const stratum::Job& stratumJob)
  {

  }

  ~WorkerJobTemplate()
  {
  }

  Job::Ptr getJobFor(const WorkerIdentifier& workerIdentifier) override
  {
    //TODO implement
    return Job::Ptr();
  }

protected:
  std::vector<uint8_t> blob_;


  std::string jobId_;

  uint32_t reservedOffset_;
  uint32_t clientNonceOffset_;
  uint32_t clientPoolOffset_;

  uint64_t difficulty_;
  uint32_t height_;
  uint64_t targetDiff_;

};

// For pool templates which can be broken down into several WorkerJobTemplates
class MasterJobTemplate : public WorkerJobTemplate
{
public:
  MasterJobTemplate(const stratum::Job& stratumJob)
    : WorkerJobTemplate(stratumJob)
  {

  }


  ~MasterJobTemplate()
  {
  }

  bool canCreateSubjacentTemplates() const override
  {
    return true;
  }

  JobTemplate::Ptr getSubjacentTemplateFor(const WorkerIdentifier& workerIdentifier) override
  {
    // not supported
    return JobTemplate::Ptr();
  }
};

// For pool jobs which can only be broken down by nice hash separation
// i.e by presetting the highest byte of the nonce at offset 39 for each miner
class NiceHashJobTemplate : public JobTemplate
{
public:
  NiceHashJobTemplate()
  {
  }

  ~NiceHashJobTemplate()
  {
  }

  Job::Ptr getJobFor(const WorkerIdentifier& workerIdentifier) override
  {
    //TODO implement
    return Job::Ptr();
  }

private:

};

// For pool jobs which can't be broken down any further
// Only one single job can be retrieved from this template
class SoloJobTemplate : public JobTemplate
{
public:
  ~SoloJobTemplate()
  {
  }

  Job::Ptr getJobFor(const WorkerIdentifier& workerIdentifier) override
  {
    //TODO implement
    return Job::Ptr();
  }

private:

};

} // namespace proxy
} // namespace ses