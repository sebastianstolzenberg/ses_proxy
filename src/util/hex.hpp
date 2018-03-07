#pragma once

#include <string>
#include <vector>
#include <boost/algorithm/hex.hpp>

namespace ses {
namespace util {

template <typename T>
inline std::string toHex(const T* begin, size_t size)
{
  std::string out;
  boost::algorithm::hex(reinterpret_cast<const uint8_t*>(begin),
                        reinterpret_cast<const uint8_t*>(begin) + (size * sizeof(T)),
                        std::back_inserter(out));
  return out;
}

template <typename T>
inline std::string toHex(const T& t)
{
  return toHex(&t, 1);
}

inline std::vector<uint8_t> fromHex(const std::string& hex)
{
  std::vector<uint8_t> data;
  boost::algorithm::unhex(hex, std::back_inserter(data));
  return data;
}

} // namespace util
} // namespace ses
