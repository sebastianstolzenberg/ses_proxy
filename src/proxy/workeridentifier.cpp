#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

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

WorkerIdentifier generateWorkerIdentifier()
{
  return boost::uuids::random_generator()();
}

} // namespace proxy
} // namespace ses