#include <boost/algorithm/string.hpp>

#include "proxy/algorithm.hpp"

namespace ses {
namespace proxy {

const char* toString(AlgorithmType algorithmType)
{
  switch (algorithmType)
  {
    case AlgorithmType::CRYPTONIGHT:
      return "cryptonight";
    case AlgorithmType::CRYPTONIGHT_LITE:
      return "cryptonight-lite";
    case AlgorithmType::CRYPTONIGHT_HEAVY:
      return "cryptonight-heavy";
    default:
      return "unknown";
  }
}

AlgorithmType toAlgorithmType(const std::string& algorithmString)
{
  AlgorithmType algorithmType = AlgorithmType::CRYPTONIGHT;
  std::string compare = algorithmString;
  boost::algorithm::to_lower(compare);
  if (compare == toString(AlgorithmType::CRYPTONIGHT_LITE))
  {
    algorithmType = AlgorithmType::CRYPTONIGHT_LITE;
  }
  else if (compare == toString(AlgorithmType::CRYPTONIGHT_HEAVY))
  {
    algorithmType = AlgorithmType::CRYPTONIGHT_HEAVY;
  }
  return algorithmType;
}

const char* toString(AlgorithmVariant algorithmVariant)
{
  switch (algorithmVariant)
  {
    case ANY: return "any"; break;
    case V0: return "0"; break;
    case V1: return "1"; break;
    case IPBC: return "ipbc"; break;
    case ALLOY: return "alloy"; break;
    case XTL: return "xtl"; break;
    case MSR: return "msr"; break;
    case XHV: return "xhv"; break;
    case RTO: return "rto"; break;
    default:
      return "unknown";
  }
}

AlgorithmVariant toAlgorithmVariant(const std::string& algorithmVariantString)
{
  AlgorithmVariant algorithmVariant = AlgorithmVariant::ANY;
  std::string compare = algorithmVariantString;
  boost::algorithm::to_lower(compare);
  for (int variant = AlgorithmVariant::ANY; variant < AlgorithmVariant::_last; variant++)
  {
    if (compare == toString(static_cast<AlgorithmVariant>(variant)))
    {
      algorithmVariant = static_cast<AlgorithmVariant>(variant);
      break;
    }
  }
  return algorithmVariant;
}

Algorithm::Algorithm()
    : algorithmType_(CRYPTONIGHT), algorithmVariant_(ANY)
{
}

Algorithm::Algorithm(AlgorithmType algorithmType_)
    : algorithmType_(algorithmType_)
{
}

Algorithm::Algorithm(AlgorithmType algorithmType_, AlgorithmVariant algorithmVariant_)
: algorithmType_( algorithmType_), algorithmVariant_(algorithmVariant_)
{
}

bool Algorithm::operator==(const Algorithm& rhs) const
{
  return algorithmType_ == rhs.algorithmType_ &&
         (algorithmVariant_ == rhs.algorithmVariant_ ||
          algorithmVariant_ == AlgorithmVariant::ANY ||
          rhs.algorithmVariant_ == AlgorithmVariant::ANY);
}

AlgorithmType Algorithm::getAlgorithmType_() const
{
  return algorithmType_;
}

AlgorithmVariant Algorithm::getAlgorithmVariant_() const
{
  return algorithmVariant_;
}

}
}