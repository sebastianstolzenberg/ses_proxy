#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/dump.hpp>

namespace ses
{
namespace log
{
void setMinimumLogLevel(boost::log::trivial::severity_level level);
std::string currentExceptionDiagnosticInformation();
} // namespace log
} // namespace ses

#define LOG_TRACE BOOST_LOG_TRIVIAL(trace)
#define LOG_DEBUG BOOST_LOG_TRIVIAL(debug)
#define LOG_INFO BOOST_LOG_TRIVIAL(info)
#define LOG_WARN BOOST_LOG_TRIVIAL(warning)
#define LOG_ERROR BOOST_LOG_TRIVIAL(error)
#define LOG_FATAL BOOST_LOG_TRIVIAL(fatal)


#define LOG_CURRENT_EXCEPTION LOG_ERROR << ses::log::currentExceptionDiagnosticInformation();

