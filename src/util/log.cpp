#include <boost/exception/diagnostic_information.hpp>
#include <boost/log/expressions.hpp>

#include "util/log.hpp"

namespace ses
{
namespace log
{
void setMinimumLogLevel(boost::log::trivial::severity_level level)
{
  boost::log::core::get()->set_filter(
      boost::log::trivial::severity >= level);
}

std::string currentExceptionDiagnosticInformation()
{
  return boost::current_exception_diagnostic_information();
}
} // namespace log
} // namespace ses