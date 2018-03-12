#pragma once

#include <string>
#include <vector>
#include <boost/algorithm/hex.hpp>
#include <boost/endian/conversion.hpp>

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

//template <typename T>
//inline std::string toHex(const T& t)
//{
//  return toHex(&t, 1);
//}

//template <typename T>
//inline T fromHex(const std::string& hex)
//{
//  T result;
//  std::memset(&result, 0, sizeof(result));
//  if (hex.size() <= 2 * sizeof(T))
//  {
//    try
//    {
//      boost::algorithm::unhex(hex, reinterpret_cast<uint8_t*>(&result));
//      result = boost::endian::big_to_native(result);
//    }
//    catch (...) {}
//  }
//  return result;
//}

inline std::vector<uint8_t> fromHex(const std::string& hex)
{
  std::vector<uint8_t> data;
  boost::algorithm::unhex(hex, std::back_inserter(data));
  return data;
}


} // namespace util
} // namespace ses
