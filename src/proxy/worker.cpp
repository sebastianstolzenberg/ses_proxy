#include "proxy/worker.hpp"

namespace ses {
namespace proxy {

bool Worker::canHandleJobTemplates() const
{
  return false;
};

void Worker::assignJobTemplate(const JobTemplate::Ptr&)
{
  throw "Not implemented";
}

} // namespace proxy
} // namespace ses
