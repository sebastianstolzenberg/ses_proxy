#pragma once

#include <sstream>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/limits.hpp>
#include <boost/algorithm/hex.hpp>

#include <string>

namespace ses {
namespace proxy {

namespace
{
template <typename T>
std::string toHex(const T* begin, size_t size)
{
  std::string out;
  boost::algorithm::hex(reinterpret_cast<const uint8_t*>(begin),
                        reinterpret_cast<const uint8_t*>(begin) + (size * sizeof(T)),
                        std::back_inserter(out));
  return out;
}

template <typename T>
std::string toHex(const T& t)
{
  return toHex(&t, 1);
}

}

uint32_t difficultyToTarget(uint64_t difficulty)
{
  boost::multiprecision::uint256_t base =
      std::numeric_limits<boost::multiprecision::uint256_t>::max();
  std::cout << "difficultyToTarget(), base, " << base << std::endl;
  std::cout << "difficultyToTarget(), hex(base), "
            <<  toHex(base.backend().limbs(), base.backend().size()) << std::endl;
  std::cout << "difficultyToTarget(), difficulty, " << difficulty << std::endl;
  std::cout << "difficultyToTarget(), hex(difficulty), " << toHex(difficulty) << std::endl;
  base /= difficulty;
  std::cout << "difficultyToTarget(), base/difficulty, " << base << std::endl;
  std::cout << "difficultyToTarget(), hex(base/difficulty), "
            << toHex(base.backend().limbs(), base.backend().size()) << std::endl;
  boost::multiprecision::limb_type* limb = base.backend().limbs();
  uint32_t target;
  memcpy(&target, reinterpret_cast<uint8_t*>(limb), sizeof(target));
  std::cout << "difficultyToTarget(), target, " << target << std::endl;
  std::cout << "difficultyToTarget(), targetHex, " << toHex(target) << std::endl;
  return target;
}


} // namespace proxy
} // namespace ses
