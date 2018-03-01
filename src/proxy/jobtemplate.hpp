#ifndef SES_PROXY_JOBTEMPLATE_HPP
#define SES_PROXY_JOBTEMPLATE_HPP

#include <memory>
#include <functional>

#include "stratum/job.hpp"
#include "proxy/job.hpp"
#include "proxy/workeridentifier.hpp"

namespace ses {
namespace proxy {

class JobTemplate : public std::enable_shared_from_this<JobTemplate>
{
public:
  typedef std::shared_ptr<JobTemplate> Ptr;

public:
  JobTemplate::Ptr create(const stratum::Job& stratumJob);

  void setJobResultHandler(const JobResult::Handler& jobResultHandler);

  virtual bool canCreateSubjacentTemplates() const;
  virtual JobTemplate::Ptr getNextSubjacentTemplate();

  //TODO set difficulty
  virtual Job::Ptr getNextJobFor(const WorkerIdentifier& workerIdentifier) = 0;

  virtual size_t numHashesFound() const = 0;
  virtual size_t currentHashRate() const = 0;

protected:
  JobTemplate() = default;
  virtual ~JobTemplate() = default;

protected:
  JobResult::Handler jobResultHandler_;
};

} // namespace proxy
} // namespace ses

#endif //SES_PROXY_JOBTEMPLATE_HPP
