#pragma once

namespace ses {
namespace proxy {

enum Algorithm
{
  ALGORITHM_CRYPTONIGHT,
  ALGORITHM_CRYPTONIGHT_LITE
};

inline const char* toString(Algorithm algorithm)
{
  switch(algorithm)
  {
    case ALGORITHM_CRYPTONIGHT: return "cryptonight";
    case ALGORITHM_CRYPTONIGHT_LITE: return "cryptonight-lite";
    default: return "unknown";
  }
}

} // namespace proxy
} // namespace ses
