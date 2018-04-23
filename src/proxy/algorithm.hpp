#pragma once

#include <string>

namespace ses {
namespace proxy {

enum Algorithm
{
  ALGORITHM_CRYPTONIGHT,
  ALGORITHM_CRYPTONIGHT_LITE,
  ALGORITHM_CRYPTONIGHT_LITE_IPBC,
  ALGORITHM_CRYPTONIGHT_HEAVY
};

const char* toString(Algorithm algorithm);

Algorithm toAlgorithm(const std::string& algoString);

} // namespace proxy
} // namespace ses
