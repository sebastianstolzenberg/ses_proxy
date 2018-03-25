#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace ses
{
namespace log
{
enum Component
{
  any,
  proxy,
  pool,
  server,
  client,
  net
};
namespace keywords {
BOOST_LOG_ATTRIBUTE_KEYWORD(component, "Component", ses::log::Component)
}
void initialize(boost::log::trivial::severity_level level, bool syslog);

std::string currentExceptionDiagnosticInformation();
} // namespace log
} // namespace ses

#define SES_LOG(lvl, cmp) \
    BOOST_LOG_TRIVIAL(lvl) << boost::log::add_value("Component", ::ses::log::cmp)

#define LOG_COMPONENT any
#define LOG_TRACE SES_LOG(trace, LOG_COMPONENT)
#define LOG_DEBUG SES_LOG(debug, LOG_COMPONENT)
#define LOG_INFO SES_LOG(info, LOG_COMPONENT)
#define LOG_WARN SES_LOG(warning, LOG_COMPONENT)
#define LOG_ERROR SES_LOG(error, LOG_COMPONENT)
#define LOG_FATAL SES_LOG(fatal, LOG_COMPONENT)


#define LOG_CURRENT_EXCEPTION LOG_ERROR << ses::log::currentExceptionDiagnosticInformation();

