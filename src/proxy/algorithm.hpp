#pragma once

#include <string>

namespace ses {
namespace proxy {

enum AlgorithmType
{
  CRYPTONIGHT,
  CRYPTONIGHT_LITE,
  CRYPTONIGHT_HEAVY
};

const char* toString(AlgorithmType algorithmType);
const char* toShortName(AlgorithmType algorithmType);

AlgorithmType toAlgorithmType(const std::string& algoString);



enum AlgorithmVariant
{
  ANY,
  V0,
  V1,
  V2,
  TUBE,
  ALLOY,
  XTL,
  MSR,
  XHV,
  RTO,
  _last
};

const char* toString(AlgorithmVariant algorithmVariant);

AlgorithmVariant toAlgorithmVariant(const std::string& algorithmVariantString);

class Algorithm
{
public:
  Algorithm();
  Algorithm(AlgorithmType algorithmType_);
  Algorithm(AlgorithmType algorithmType_, AlgorithmVariant algorithmVariant_);

  bool operator==(const Algorithm& rhs) const;
  bool operator!=(const Algorithm& rhs) const;

  AlgorithmType getAlgorithmType_() const;
  AlgorithmVariant getAlgorithmVariant_() const;

  // for logging
  template <class STREAM>
  friend STREAM& operator<< (STREAM& stream, Algorithm const& val)
  {
    stream << "Algorithm(" << toString(val.algorithmType_)
           << ", variant " << toString(val.algorithmVariant_) << ")";
    return stream;
  }

private:
  AlgorithmType algorithmType_;
  AlgorithmVariant algorithmVariant_;
};
} // namespace proxy
} // namespace ses
