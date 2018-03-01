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

  Job(const std::string& id, const std::string& jobId, const std::string& blobHexString,
      const std::string& targetHexString,
      const std::string& blocktemplate_blob, const std::string& difficulty, const std::string& height,
      const std::string& reserved_offset, const std::string& client_nonce_offset,
      const std::string& client_pool_offset, const std::string& target_diff, const std::string& target_diff_hex,
      const std::string& job_id);

  const std::string& getId() const;
  const std::string& getJobId() const;
  const std::string& getBlob() const;
  const std::string& getTarget() const;

  const std::string& getBlocktemplate_blob() const;
  const std::string& getDifficulty() const;
  const std::string& getHeight() const;
  const std::string& getReserved_offset() const;
  const std::string& getClientNonceOffset() const;
  const std::string& getClientPoolOffset() const;
  const std::string& getTargetDiff() const;
  const std::string& getTargetDiffHex() const;

  bool isBlockTemplate() const;

private:
  std::string blob_;
  std::string jobId_;
  std::string target_;
  std::string id_;

  //nodejs specific fields
  std::string blocktemplateBlob_;
  std::string difficulty_;
  std::string height_;
  std::string reservedOffset_;
  std::string clientNonceOffset_;
  std::string clientPoolOffset_;
  std::string targetDiff_;
  std::string targetDiffHex_;
};

} // namespace stratum
} // namespace ses

#endif //SES_STRATUM_JOB_HPP
