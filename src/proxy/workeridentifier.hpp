#pragma once

#include <string>
#include <boost/uuid/uuid.hpp>

namespace ses {
namespace proxy {

typedef boost::uuids::uuid WorkerIdentifier;

std::string toString(const WorkerIdentifier& workerIdentifier);
WorkerIdentifier toWorkerIdentifier(const std::string& workerIdentifier);

WorkerIdentifier generateWorkerIdentifier();

} // namespace proxy
} // namespace ses
