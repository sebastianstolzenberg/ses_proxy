#include <boost/algorithm/string.hpp>

#include "proxy/algorithm.hpp"

namespace ses {
namespace proxy {

const char* toString(Algorithm algorithm)
{
  switch (algorithm)
  {
    case ALGORITHM_CRYPTONIGHT:
      return "cryptonight";
    case ALGORITHM_CRYPTONIGHT_LITE:
      return "cryptonight-lite";
    case ALGORITHM_CRYPTONIGHT_LITE_IPBC:
      return "cryptonight-lite-ipbc";
    case ALGORITHM_CRYPTONIGHT_HEAVY:
      return "cryptonight-heavy";
    default:
      return "unknown";
  }
}

Algorithm toAlgorithm(const std::string& algorithmString)
{
  Algorithm algorithm = ALGORITHM_CRYPTONIGHT;
  std::string compare = algorithmString;
  boost::algorithm::to_lower(compare);
  if (compare == toString(ALGORITHM_CRYPTONIGHT_LITE))
  {
    algorithm = ALGORITHM_CRYPTONIGHT_LITE;
  }
  else if (compare == toString(ALGORITHM_CRYPTONIGHT_LITE_IPBC))
  {
    algorithm = ALGORITHM_CRYPTONIGHT_LITE_IPBC;
  }
  else if (compare == toString(ALGORITHM_CRYPTONIGHT_HEAVY))
  {
    algorithm = ALGORITHM_CRYPTONIGHT_HEAVY;
  }
  return algorithm;
}

}
}