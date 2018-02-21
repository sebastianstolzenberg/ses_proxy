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
  typedef std::shared_ptr<Job> Ptr;

public:
  Job(const std::string& blobHexString, const std::string& jobId, const std::string& targetHexString,
      const std::string& id);

  bool isValid() const;

  const std::vector<uint8_t>& getBlob() const;
  std::string getBlobHexString() const;

  const std::string& getJobId() const;

  uint64_t getTarget() const;
  std::string getTargetHexString() const;

  const std::string& getId() const;

  uint32_t getNonce() const;
  void setNonce(uint32_t nonce);

private:
  std::vector<uint8_t> blob_;
  std::string jobId_;
  uint64_t target_;
  std::string id_;
};

} // namespace stratum
} // namespace ses

#endif //SES_STRATUM_JOB_HPP
