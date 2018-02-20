#ifndef SES_STRATUM_JOB_HPP
#define SES_STRATUM_JOB_HPP

#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace ses {
namespace stratum {

class Job
{
public:
  Job(const std::string& blob, const std::string& jobId, const std::string& target, const std::string& id);

  const std::vector& getBlob() const;

  const std::string& getJobId() const;

  uint64_t getTarget() const;

  const std::string& getId() const;

private:
  std::vector blob_;
  std::string jobId_;
  uint64_t target_;
  std::string id_;
};

} // namespace stratum
} // namespace ses

#endif //SES_STRATUM_JOB_HPP
