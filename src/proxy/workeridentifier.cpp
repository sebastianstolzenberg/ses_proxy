#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include "proxy/workeridentifier.hpp"

namespace ses {
namespace proxy {

std::string toString(const WorkerIdentifier& workerIdentifier)
{
  return boost::lexical_cast<std::string>(workerIdentifier);
}

WorkerIdentifier toWorkerIdentifier(const std::string& workerIdentifier)
{
  return boost::lexical_cast<WorkerIdentifier>(workerIdentifier);
}


} // namespace proxy
} // namespace ses