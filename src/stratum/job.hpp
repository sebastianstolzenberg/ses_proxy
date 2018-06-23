#pragma once

#include <string>
#include <vector>
#include <memory>

namespace ses {
namespace stratum {

class Job
{
public:
  Job() = default;
  Job(const std::string& id, const std::string& jobId, const std::string& algo, const std::string& variant,
      const std::string& blobHexString, const std::string& targetHexString);

//  Job(const std::string& blocktemplate_blob,
//      const std::string& difficulty, const std::string& height,
//      const std::string& reserved_offset, const std::string& client_nonce_offset);

  Job(const std::string& id, const std::string& jobId, const std::string& blobHexString,
      const std::string& targetHexString,
      const std::string& blocktemplate_blob, const std::string& difficulty, const std::string& height,
      const std::string& reserved_offset, const std::string& client_nonce_offset,
      const std::string& client_pool_offset, const std::string& target_diff, const std::string& target_diff_hex,
      const std::string& job_id);

  const std::string& getId() const;
  void setId(const std::string& id);

  const std::string& getJobIdentifier() const;
  void setJobIdentifier(const std::string& jobId);

  const std::string& getAlgo() const;
  const std::string& getVariant() const;

  const std::string& getBlob() const;

  const std::string& getTarget() const;
  void setTarget(const std::string& target);
  const std::string& getBlocktemplateBlob() const;

  const std::string& getDifficulty() const;
  void setDifficulty(const std::string& difficulty);
  const std::string& getHeight() const;
  void setHeight(const std::string& height);

  const std::string& getReservedOffset() const;
  void setReservedOffset(const std::string& reservedOffset);
  const std::string& getClientNonceOffset() const;
  void setClientNonceOffset(const std::string& clientNonceOffset);
  const std::string& getClientPoolOffset() const;
  void setClientPoolOffset(const std::string& clientPoolOffset);

  const std::string& getTargetDiff() const;
  const std::string& getTargetDiffHex() const;

  bool isBlockTemplate() const;

private:
  std::string blob_;
  std::string jobId_;
  std::string target_;
  std::string id_;
  std::string algo_;
  std::string variant_;

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
