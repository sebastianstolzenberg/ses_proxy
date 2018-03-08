#include <cstdint>
#include <mutex>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/random_generator.hpp>

#include "proxy/jobtemplateworker.hpp"
#include "proxy/jobtemplatemaster.hpp"
#include "proxy/jobtemplatenicehash.hpp"
#include "proxy/jobtemplatesolo.hpp"
#include "util/log.hpp"

namespace ses {
namespace proxy {

namespace {
uint64_t parseTarget(const std::string& targetHexString)
{
  uint64_t target = 0;
  if (targetHexString.size() <= 2 * sizeof(uint64_t))
  {
    boost::algorithm::unhex(targetHexString, reinterpret_cast<uint8_t*>(&target));
    if (targetHexString.size() <= 2 * sizeof(uint32_t))
    {
      // multiplication necessary
      uint32_t halfTarget = 0;
      boost::algorithm::unhex(targetHexString, reinterpret_cast<uint8_t*>(&halfTarget));
      target = halfTarget;
      target = 0xFFFFFFFFFFFFFFFFULL / (0xFFFFFFFFULL / target);
    }
    else
    {
      boost::algorithm::unhex(targetHexString, reinterpret_cast<uint8_t*>(&target));
    }
  }
  return target;
}
}

JobTemplate::Ptr JobTemplate::create(const stratum::Job& stratumJob)
{
  JobTemplate::Ptr jobTemplate;

  Blob blob(stratumJob);
  WorkerIdentifier identifier = toWorkerIdentifier(stratumJob.getId());
  std::string jobIdentifier = stratumJob.getJobIdentifier();

  if (blob.isTemplate())
  {
    // JobTemplate case
    uint32_t difficulty = boost::lexical_cast<uint64_t>(stratumJob.getDifficulty());
    uint32_t height = boost::lexical_cast<uint32_t>(stratumJob.getHeight());
    uint32_t targetDiff = boost::lexical_cast<uint32_t>(stratumJob.getTargetDiff());
    if (blob.hasClientPoolOffset())
    {
      jobTemplate =
          std::make_shared<MasterJobTemplate>(identifier, jobIdentifier, std::move(blob),
                                              difficulty, height, targetDiff);
    }
    else
    {
    }

  }
  else
  {
    // Job case
    uint64_t target = parseTarget(stratumJob.getTarget());
    if (blob.getNiceHash() == static_cast<uint8_t>(0))
    {
      jobTemplate = std::make_shared<NiceHashJobTemplate>(identifier, jobIdentifier, std::move(blob), target);
    }
    else
    {
      jobTemplate = std::make_shared<SoloJobTemplate>(identifier, jobIdentifier, std::move(blob), target);
    }
  }

  return jobTemplate;
}

} // namespace proxy
} // namespace ses