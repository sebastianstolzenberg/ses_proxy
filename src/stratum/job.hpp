#ifndef SES_STRATUM_JOB_HPP
#define SES_STRATUM_JOB_HPP

#include <string>
#include <vector>
#include <memory>

namespace ses {
namespace stratum {

class Job
{
public:
  Job(const std::string& id, const std::string& jobId, const std::string& blobHexString,
      const std::string& targetHexString);

  const std::string& getId() const;
  const std::string& getJobId() const;
  const std::string& getBlob() const;
  const std::string& getTarget() const;

private:
  std::string blob_;
  std::string jobId_;
  std::string target_;
  std::string id_;
};

} // namespace stratum
} // namespace ses

#endif //SES_STRATUM_JOB_HPP
