#include <boost/endian/conversion.hpp>
#include <boost/algorithm/hex.hpp>

#include "util/target.hpp"

namespace ses {
namespace util {
namespace {

//uint32_t target64ToTarget32(uint64_t target64)
//{
//  return *(reinterpret_cast<uint32_t*>(&target64) + 1);
//}

uint64_t target32ToTarget64(uint32_t target32)
{
  uint64_t target64 = 0;
  *(reinterpret_cast<uint32_t*>(&target64) + 1) = target32;
  return target64;
}

uint64_t parseTarget(const std::string& targetHexString)
{
  uint64_t target = 0;
  if (targetHexString.size() <= 2 * sizeof(uint64_t))
  {
    boost::algorithm::unhex(targetHexString, reinterpret_cast<uint8_t*>(&target));
//    target = boost::endian::big_to_native(target);
    target <<= 8 * (sizeof(target) - targetHexString.size() / 2);
  }
  return target;
}

}

Target::Target(uint64_t target) : target_(target) {}

Target::Target(uint32_t target) : target_(target32ToTarget64(target)) {}

Target::Target(const std::string& targetHexString) : target_(parseTarget(targetHexString)) {}

bool Target::operator==(const Target& other) const
{
  return target_ == other.target_;
}

uint64_t Target::getRaw() const
{
  return target_;
}

Target& Target::trim(size_t numberOfRelevantBytes)
{
  const uint64_t msb64 = 0xff00000000000000;
  uint64_t mask = 0;
  while (numberOfRelevantBytes--)
  {
    mask = (mask >> 8) | msb64;
  }
  target_ &= mask;
  return *this;
}

std::string Target::toHexString(size_t numberOfBytes) const
{
  std::string targetHex;
//  uint64_t target = boost::endian::native_to_big(target_);
  numberOfBytes = std::min(numberOfBytes, sizeof(target_));
  size_t offset = sizeof(target_) - numberOfBytes;
  boost::algorithm::hex_lower(reinterpret_cast<const uint8_t*>(&target_) + offset,
                              reinterpret_cast<const uint8_t*>(&target_) + sizeof(target_),
                              std::back_inserter(targetHex));
  targetHex.resize(std::min(numberOfBytes*2, targetHex.size()));
  return targetHex;
}

} // namespace util
} // namespace ses