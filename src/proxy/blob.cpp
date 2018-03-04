#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>

#include "blob.hpp"

namespace ses {
namespace proxy {

namespace {
const size_t NONCE_OFFSET = 39;

//TODO check if isValid is needed
//bool Job::isValid() const
//{
//  const size_t BLOB_SIZE_MAX = 84;
//  const size_t BLOB_SIZE_MIN = 76;
//  return target_ != 0 && blob_.size() >= BLOB_SIZE_MIN && blob_.size() <= BLOB_SIZE_MAX;
//}
//

std::vector<uint8_t> parseBlob(const std::string& blobHexString)
{
  std::vector<uint8_t> blob;
  boost::algorithm::unhex(blobHexString, std::back_inserter(blob));
  return blob;
}
}

Blob::Blob(const stratum::Job& job) :
  isTemplate_(!job.getBlocktemplateBlob().empty()),
  blob_(parseBlob(isTemplate_ ? job.getBlocktemplateBlob() : job.getBlob())),
  nonceOffset_(NONCE_OFFSET),
  reservedOffset_(job.getReservedOffset().empty() ?
                  std::numeric_limits<uint32_t>::max() :
                  boost::lexical_cast<uint32_t>(job.getReservedOffset())),
  clientNonceOffset_(job.getClientNonceOffset().empty() ?
                     std::numeric_limits<uint32_t>::max() :
                     boost::lexical_cast<uint32_t>(job.getClientNonceOffset())),
  clientPoolOffset_(job.getClientPoolOffset().empty() ?
                    std::numeric_limits<uint32_t>::max() :
                    boost::lexical_cast<uint32_t>(job.getClientPoolOffset()))
{
}

stratum::Job Blob::asStratumJob() const
{
  return stratum::Job("", // id
                      "", // jobId
                      isTemplate() ? "" : toHexString(), // blobHexString
                      "", //  targetHexString
                      isTemplate() ? toHexString() : "", // blobHexString
                      "", "",
                      hasReservedOffset() ? std::to_string(reservedOffset_) : "",
                      hasClientNonceOffset() ? std::to_string(clientNonceOffset_) : "",
                      hasClientPoolOffset() ? std::to_string(clientPoolOffset_) : "",
                      "", "", "");
}

std::string Blob::toHexString() const
{
  std::string blobHex;
  boost::algorithm::hex_lower(blob_, std::back_inserter(blobHex));
  return blobHex;
}

bool Blob::isTemplate() const
{
  return isTemplate_;
}

uint8_t Blob::getNiceHash() const
{
  return *(reinterpret_cast<const uint8_t*>(blob_.data() + nonceOffset_));
}

void Blob::setNiceHash(uint8_t niceHash)
{
  *(reinterpret_cast<uint8_t*>(blob_.data() + nonceOffset_)) = niceHash;
}

uint32_t Blob::getNonce() const
{
  return *(reinterpret_cast<const uint32_t*>(blob_.data() + nonceOffset_));
}

void Blob::setNonce(uint32_t nonce)
{
  *(reinterpret_cast<uint32_t*>(blob_.data() + nonceOffset_)) = nonce;
}

bool Blob::hasReservedOffset() const
{
  return reservedOffset_ < std::numeric_limits<uint32_t>::max();
}

uint32_t Blob::getReservedNonce() const
{
  uint32_t reservedNonce = 0;
  if (hasReservedOffset())
  {
    reservedNonce = *(reinterpret_cast<const uint32_t*>(blob_.data() + reservedOffset_));
  }
  return reservedNonce;
}

void Blob::setReservedNonce(uint32_t reservedNonce)
{
  if (hasReservedOffset())
  {
    *(reinterpret_cast<uint32_t*>(blob_.data() + reservedOffset_)) = reservedNonce;
  }
}

bool Blob::hasClientNonceOffset() const
{
  return clientNonceOffset_ < std::numeric_limits<uint32_t>::max();
}

uint32_t Blob::getClientNonce() const
{
  uint32_t clientNonce = 0;
  if (hasClientNonceOffset())
  {
    clientNonce = *(reinterpret_cast<const uint32_t*>(blob_.data() + clientNonceOffset_));
  }
  return clientNonce;
}

void Blob::setClientNonce(uint32_t clientNonce)
{
  if (hasClientNonceOffset())
  {
    *(reinterpret_cast<uint32_t*>(blob_.data() + clientNonceOffset_)) = clientNonce;
  }
}

bool Blob::hasClientPoolOffset() const
{
  return clientPoolOffset_ < std::numeric_limits<uint32_t>::max();
}

uint32_t Blob::getClientPool() const
{
  uint32_t clientPool = 0;
  if (hasClientPoolOffset())
  {
    clientPool = *(reinterpret_cast<const uint32_t*>(blob_.data() + clientPoolOffset_));
  }
  return clientPool;
}

void Blob::setClientPool(uint32_t clientPool)
{
  if (hasClientPoolOffset())
  {
    *(reinterpret_cast<uint32_t*>(blob_.data() + clientPoolOffset_)) = clientPool;
  }
}

} // namespace proxy
} // namespace ses
