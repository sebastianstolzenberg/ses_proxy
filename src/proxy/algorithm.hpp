#pragma once

#include <string>

namespace ses {
namespace proxy {

enum Algorithm
{
  CRYPTONIGHT,
  CRYPTONIGHT_LITE,
  CRYPTONIGHT_HEAVY
};

const char* toString(Algorithm algorithm);

Algorithm toAlgorithm(const std::string& algoString);



enum AlgorithmVariant
{
  ANY,
  V0,
  V1,
  IPBC,
  ALLOY,
  XTL
};

const char* toString(AlgorithmVariant algorithmVariant);

AlgorithmVariant toAlgorithmVariant(const std::string& algorithmVariantString);


} // namespace proxy
} // namespace ses
