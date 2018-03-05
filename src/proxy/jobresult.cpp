#include <boost/algorithm/hex.hpp>

#include "jobresult.hpp"

namespace ses {
namespace proxy {

namespace {
uint32_t parseNonce(const std::string& nonceHexString)
{
  uint32_t nonce = 0;
  boost::algorithm::unhex(nonceHexString, reinterpret_cast<uint8_t*>(&nonce));
  return nonce;
}

JobResult::Hash parseHash(const std::string& hashHexString)
{
  JobResult::Hash hash;
  boost::algorithm::unhex(hashHexString, hash.begin());
  return hash;
}
}

JobResult::JobResult(const std::string& jobId, const std::string& nonce, const std::string& hash)
  : jobId_(jobId), nonce_(parseNonce(nonce)), hash_(parseHash(hash)), isNodeJsResult_(false)
{
}

JobResult::JobResult(const std::string& btId, const std::string& nonce, const std::string& resultHash,
                     const std::string& workerNonce, const std::string& poolNonce)
  : jobId_(btId)
  , nonce_(parseNonce(nonce))
  , hash_(parseHash(resultHash))
  , workerNonce_(parseNonce(workerNonce))
  , poolNonce_(parseNonce(poolNonce))
  , isNodeJsResult_(true)
{
}

bool JobResult::isNodeJsResult() const
{
  return isNodeJsResult_;
}

std::string JobResult::getNonceHexString() const
{
  std::string hex;
  boost::algorithm::hex_lower(reinterpret_cast<const uint8_t*>(&nonce_),
                        reinterpret_cast<const uint8_t*>(&nonce_) + sizeof(nonce_),
                        std::back_inserter(hex));
  return hex;
}

std::string JobResult::getHashHexString() const
{
  std::string hex;
  boost::algorithm::hex_lower(hash_, std::back_inserter(hex));
  return hex;
}

std::string JobResult::getWorkerNonceHexString() const
{
  std::string hex;
  boost::algorithm::hex_lower(reinterpret_cast<const uint8_t*>(&workerNonce_),
                              reinterpret_cast<const uint8_t*>(&workerNonce_) + sizeof(workerNonce_),
                              std::back_inserter(hex));
  return hex;
}

std::string JobResult::getPoolNonceHexString() const
{
  std::string hex;
  boost::algorithm::hex_lower(reinterpret_cast<const uint8_t*>(&poolNonce_),
                              reinterpret_cast<const uint8_t*>(&poolNonce_) + sizeof(poolNonce_),
                              std::back_inserter(hex));
  return hex;
}

const std::string& JobResult::getJobIdentifier() const
{
  return jobId_;
}

void JobResult::setJobId(const std::string& jobId)
{
  jobId_ = jobId;
}

uint32_t JobResult::getNonce() const
{
  return nonce_;
}

const JobResult::Hash& JobResult::getHash() const
{
  return hash_;
}

uint32_t JobResult::getWorkerNonce() const
{
  return workerNonce_;
}

void JobResult::setWorkerNonce(uint32_t workerNonce)
{
  workerNonce_ = workerNonce;
}

uint32_t JobResult::getPoolNonce() const
{
  return poolNonce_;
}

void JobResult::setPoolNonce(uint32_t poolNonce)
{
  poolNonce_ = poolNonce;
}

uint8_t JobResult::getNiceHash() const
{
  return static_cast<uint8_t>((getNonce() >> (sizeof(uint32_t) - sizeof(uint8_t))) & 0x000000ff);
}

} // namespace proxy
} // namespace ses