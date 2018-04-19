#pragma once

#include <cstdint>
#include <string>

namespace ses {
namespace util {

class Target
{
public:
  explicit Target(uint64_t target);
  explicit Target(uint32_t target);
  Target(const std::string& targetHexString);

  bool operator==(const Target& other) const;

  bool isNull() const;
  uint64_t getRaw() const;
  Target& trim(size_t numberOfRelevantBytes = 4);
  std::string toHexString(size_t numberOfBytes = 4) const;

private:
  uint64_t target_;
};

} // namespace util
} // namespace ses
