#pragma once

#include "stratum/job.hpp"

namespace ses {
namespace proxy {

class Blob
{
public:
  Blob(const stratum::Job& job);

  const std::vector<uint8_t>& blob() const;
  void convertToHashBlob();
  stratum::Job asStratumJob() const;

  std::string toHexString() const;

  bool isTemplate() const;

  uint8_t getNiceHash() const;
  void setNiceHash(uint8_t niceHash);

  uint32_t getNonce() const;
  void setNonce(uint32_t nonce);

  bool hasReservedOffset() const;
  uint32_t getReservedNonce() const;
  void setReservedNonce(uint32_t reservedNonce);

  bool hasClientNonceOffset() const;
  uint32_t getClientNonce() const;
  void setClientNonce(uint32_t clientNonce);

  bool hasClientPoolOffset() const;
  uint32_t getClientPool() const;
  void setClientPool(uint32_t clientPool);

private:
  bool isTemplate_;
  std::vector<uint8_t> blob_;
  uint32_t nonceOffset_;
  uint32_t reservedOffset_;
  uint32_t clientNonceOffset_;
  uint32_t clientPoolOffset_;
};

} // namespace proxy
} // namespace ses
