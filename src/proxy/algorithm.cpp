#include <boost/algorithm/string.hpp>

#include "proxy/algorithm.hpp"

namespace ses {
namespace proxy {

const char* toString(Algorithm algorithm)
{
  switch (algorithm)
  {
    case Algorithm::CRYPTONIGHT:
      return "cryptonight";
    case Algorithm::CRYPTONIGHT_LITE:
      return "cryptonight-lite";
    case Algorithm::CRYPTONIGHT_HEAVY:
      return "cryptonight-heavy";
    default:
      return "unknown";
  }
}

Algorithm toAlgorithm(const std::string& algorithmString)
{
  Algorithm algorithm = Algorithm::CRYPTONIGHT;
  std::string compare = algorithmString;
  boost::algorithm::to_lower(compare);
  if (compare == toString(Algorithm::CRYPTONIGHT_LITE))
  {
    algorithm = Algorithm::CRYPTONIGHT_LITE;
  }
  else if (compare == toString(Algorithm::CRYPTONIGHT_HEAVY))
  {
    algorithm = Algorithm::CRYPTONIGHT_HEAVY;
  }
  return algorithm;
}

const char* toString(AlgorithmVariant algorithmVariant)
{
  switch (algorithmVariant)
  {
    case ANY: return "-1"; break;
    case V0: return "0"; break;
    case V1: return "1"; break;
    case IPBC: return "ipbc"; break;
    case ALLOY: return "alloy"; break;
    case XTL: return "xtl"; break;
    default:
      return "unknown";
  }
}

AlgorithmVariant toAlgorithmVariant(const std::string& algorithmVariantString)
{
  AlgorithmVariant algorithmVariant = AlgorithmVariant::ANY;
  std::string compare = algorithmVariantString;
  boost::algorithm::to_lower(compare);
  for (int variant = AlgorithmVariant::ANY; variant <= AlgorithmVariant::XTL; variant++)
  {
    if (compare == toString(static_cast<AlgorithmVariant>(variant)))
    {
      algorithmVariant = static_cast<AlgorithmVariant>(variant);
      break;
    }
  }
  return algorithmVariant;
}

}
}