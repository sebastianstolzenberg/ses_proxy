#include <cstdint>
#include <mutex>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/endian/conversion.hpp>

#include "proxy/jobtemplateworker.hpp"
#include "proxy/jobtemplatemaster.hpp"
#include "proxy/jobtemplatenicehash.hpp"
#include "proxy/jobtemplatesolo.hpp"
#include "util/target.hpp"
#include "util/log.hpp"

namespace ses {
namespace proxy {

JobTemplate::Ptr JobTemplate::create(const std::string& workerIdentifier,
                                     const stratum::Job& stratumJob)
{
  JobTemplate::Ptr jobTemplate;

  Blob blob(stratumJob);
  std::string identifier;
  if (!workerIdentifier.empty())
  {
    identifier = workerIdentifier;
  }
  else if (!stratumJob.getId().empty())
  {
    identifier = stratumJob.getId();
  }
  else
  {
    identifier = toString(generateWorkerIdentifier());
  }
  std::string jobIdentifier = stratumJob.getJobIdentifier();

  if (blob.isTemplate())
  {
    // JobTemplate case
    uint64_t difficulty = boost::lexical_cast<uint64_t>(stratumJob.getDifficulty());
    uint32_t height = boost::lexical_cast<uint32_t>(stratumJob.getHeight());
    uint32_t targetDifficulty = boost::lexical_cast<uint32_t>(stratumJob.getTargetDiff());
    if (blob.hasClientPoolOffset())
    {
      jobTemplate =
          std::make_shared<MasterJobTemplate>(identifier, jobIdentifier, std::move(blob),
                                              difficulty, height, targetDifficulty);
    }
    else
    {
    }

  }
  else
  {
    // Job case
    util::Target target(stratumJob.getTarget());
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