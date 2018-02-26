#ifndef SES_PROXY_JOBRESULT_HPP
#define SES_PROXY_JOBRESULT_HPP

#include <string>
#include <array>

namespace ses {
namespace proxy {

class JobResult
{
public:
  enum
  {
    HASH_SIZE_BYTES = 32
  };
  typedef std::array<uint8_t, HASH_SIZE_BYTES> Hash;

public:
  JobResult(const std::string& jobId, const std::string& nonce, const std::string& hash);

  std::string getNonceHexString() const;
  std::string getHashHexString() const;

  const std::string& getJobId() const;
  void setJobId(const std::string& jobId);
  uint32_t getNonce() const;
  const Hash& getHash() const;

  uint8_t getNiceHash() const;

private:
  std::string jobId_;
  uint32_t nonce_;
  Hash hash_;
};

} // namespace proxy
} // namespace ses

#endif //SES_PROXY_JOBRESULT_HPP