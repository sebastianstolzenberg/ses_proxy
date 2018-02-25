#ifndef SES_PROXY_JOBSOURCE_HPP
#define SES_PROXY_JOBSOURCE_HPP

#include <memory>
#include <boost/noncopyable.hpp>

#include "proxy/job.hpp"
#include "proxy/jobresult.hpp"
#include "proxy/workeridentifier.hpp"

namespace ses {
namespace proxy {

class JobSource : private boost::noncopyable
{
public:
  typedef std::shared_ptr<JobSource> Ptr;

protected:
  JobSource() = default;
  virtual ~JobSource() = default;

public:
  virtual Job::Ptr getJob(const WorkerIdentifier& workerIdentifier) = 0;
  virtual Job::SubmitStatus submitJobResult(const WorkerIdentifier& workerIdentifier, const JobResult& jobResult) = 0;
};

} // namespace proxy
} // namespace ses


#endif //SES_PROXY_JOBSOURCE_HPP
