#pragma once

#include <vector>
#include <set>

#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#define COLOR_NC           "\e[0m" // No Color
#define COLOR_WHITE        "\e[1;37m"
#define COLOR_BLACK        "\e[0;30m"
#define COLOR_BLUE         "\e[0;34m"
#define COLOR_LIGHT_BLUE   "\e[1;34m"
#define COLOR_GREEN        "\e[0;32m"
#define COLOR_LIGHT_GREEN  "\e[1;32m"
#define COLOR_CYAN         "\e[0;36m"
#define COLOR_LIGHT_CYAN   "\e[1;36m"
#define COLOR_RED          "\e[0;31m"
#define COLOR_LIGHT_RED    "\e[1;31m"
#define COLOR_PURPLE       "\e[0;35m"
#define COLOR_LIGHT_PURPLE "\e[1;35m"
#define COLOR_BROWN        "\e[0;33m"
#define COLOR_YELLOW       "\e[1;33m"
#define COLOR_GRAY         "\e[0;30m"
#define COLOR_LIGHT_GRAY   "\e[0;37m"

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
void initialize(bool syslog);

void setLogLevel(boost::log::trivial::severity_level level);

std::string currentExceptionDiagnosticInformation();

template <typename OutStream, typename Range>
inline OutStream& logRange(OutStream& out, const Range& range)
{
  out << "{";
  typename Range::const_iterator current = range.begin();
  typename Range::const_iterator next = current;
  ++next;
  typename Range::const_iterator end = range.end();
  for (;current != end; current = next++)
  {
    out << *current;
    if (next != end)
    {
      out << ", ";
    }
  }
  out << "}";
  return out;
}

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

namespace std
{
// helper for logging vector contents
template <typename OutStream, typename T>
inline OutStream& operator<<(OutStream& out, const vector<T>& range)
{
  return ses::log::logRange(out, range);
}
template <typename OutStream, typename T>
inline OutStream& operator<<(OutStream& out, const set<T>& range)
{
  return ses::log::logRange(out, range);
}
}
