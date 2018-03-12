#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/limits.hpp>

#include "util/difficulty.hpp"
#include "util/hex.hpp"
#include "util/log.hpp"

namespace ses {
namespace util {

namespace {
inline std::string toHex(const boost::multiprecision::uint256_t& t)
{
  return ses::util::toHex<boost::multiprecision::limb_type>(t.backend().limbs(), t.backend().size());
}

boost::multiprecision::uint256_t uint256FromTarget(uint64_t target)
{
  boost::multiprecision::uint256_t bigNum = std::numeric_limits<boost::multiprecision::uint256_t>::max();
  bigNum = std::numeric_limits<boost::multiprecision::uint256_t>::max();
  uint8_t* limb = reinterpret_cast<uint8_t*>(bigNum.backend().limbs());
  size_t limbSize = sizeof(boost::multiprecision::limb_type) * bigNum.backend().size();
  uint8_t* targetInLimb = limb + limbSize - sizeof(target);
  memcpy(targetInLimb, &target, sizeof(target));
  memset(limb, 0, limbSize - sizeof(target));
  return bigNum;
}

uint64_t extractTargetFromUint256(const boost::multiprecision::uint256_t& bigNum)
{
  uint64_t target;
  const uint8_t* limb = reinterpret_cast<const uint8_t*>(bigNum.backend().limbs());
  size_t limbSize = sizeof(boost::multiprecision::limb_type) * bigNum.backend().size();
  const uint8_t* targetInLimb = limb + limbSize - sizeof(target);
  memcpy(&target, targetInLimb, sizeof(target));
  return target;
}
}

uint32_t targetToDifficulty(const Target& target)
{
  return (std::numeric_limits<boost::multiprecision::uint256_t>::max() /
          uint256FromTarget(target.getRaw())).convert_to<uint32_t>();
}

Target difficultyToTarget(uint32_t difficulty)
{
  return Target(extractTargetFromUint256(std::numeric_limits<boost::multiprecision::uint256_t>::max() / difficulty));
}

uint32_t difficultyFromHashBuffer(const uint8_t* data, size_t size)
{
  boost::multiprecision::uint256_t base =
    std::numeric_limits<boost::multiprecision::uint256_t>::max();
  boost::multiprecision::uint256_t hash;
  boost::multiprecision::import_bits(hash, data, data + size, 0, false);
//  LOG_DEBUG << "targetToDifficulty(), hash, " << hash;
//  LOG_DEBUG << "targetToDifficulty(), hex(hash), " << toHex(hash);
  base /= hash;
//  LOG_DEBUG << "targetToDifficulty(), base/hash, " << base;
//  LOG_DEBUG << "targetToDifficulty(), hex(base/hash), " << toHex(base);
  return base.convert_to<uint32_t>();
}

} // namespace util
} // namespace ses
