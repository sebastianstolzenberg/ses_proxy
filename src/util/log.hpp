#pragma once

#include <boost/log/trivial.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/log/expressions.hpp>

#define LOG_TRACE BOOST_LOG_TRIVIAL(trace)
#define LOG_DEBUG BOOST_LOG_TRIVIAL(debug)
#define LOG_INFO BOOST_LOG_TRIVIAL(info)
#define LOG_WARN BOOST_LOG_TRIVIAL(warning)
#define LOG_ERROR BOOST_LOG_TRIVIAL(error)
#define LOG_FATAL BOOST_LOG_TRIVIAL(fatal)

#define LOG_CURRENT_EXCEPTION LOG_ERROR << boost::current_exception_diagnostic_information()

namespace ses
{
namespace log
{
inline void setMinimumLogLevel(boost::log::trivial::severity_level level)
{
  boost::log::core::get()->set_filter(
      boost::log::trivial::severity >= level);
}
} // namespace log
} // namespace ses