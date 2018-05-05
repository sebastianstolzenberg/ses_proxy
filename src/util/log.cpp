#define BOOST_LOG_USE_NATIVE_SYSLOG

#include <boost/exception/diagnostic_information.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include "util/log.hpp"


std::ostream& operator<<(std::ostream& strm, ::ses::log::Component component);

namespace ses
{
namespace log
{
namespace {
const char* to_string(Component component)
{
  static const char* strings[] =
    {
      "Any    ",
      "Proxy  ",
      "Pool   ",
      "Server ",
      "Client "
    };

  if (static_cast< std::size_t >(component) < sizeof(strings) / sizeof(*strings))
    return strings[component];
  else
    return nullptr;
}

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

void formatMessagePart(boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
{
  auto severity = rec[boost::log::trivial::severity].get();
  switch (severity)
  {
    case boost::log::trivial::trace:   strm << COLOR_GRAY; break;
    case boost::log::trivial::warning: strm << COLOR_BROWN; break;
    case boost::log::trivial::error:   strm << COLOR_RED; break;
    case boost::log::trivial::fatal:   strm << COLOR_LIGHT_RED; break;
    case boost::log::trivial::debug:
    case boost::log::trivial::info:
    default:
      break;
  }
  strm << "[" << std::left << std::setw(7) << severity << "] ";
  switch (severity)
  {
    case boost::log::trivial::trace:
    case boost::log::trivial::warning:
    case boost::log::trivial::error:
    case boost::log::trivial::fatal:
      strm << COLOR_NC;
      break;
    case boost::log::trivial::debug:
    case boost::log::trivial::info:
    default:
      break;
  }

  auto component = rec[ses::log::keywords::component];
  switch (component ? component.get() : ses::log::any)
  {
    case ses::log::any:    strm <<              "Any    - "; break;
    case ses::log::proxy:  strm << COLOR_BLUE   "Proxy  - "; break;
    case ses::log::pool:   strm << COLOR_PURPLE "Pool   - "; break;
    case ses::log::server: strm << COLOR_CYAN   "Server - "; break;
    case ses::log::client: strm << COLOR_GREEN  "Client - "; break;
    case ses::log::net:    strm << COLOR_YELLOW "Net    - "; break;
    default: break;
  }
  strm << rec[boost::log::expressions::smessage];
  switch (component ? component.get() : ses::log::any)
  {
    case ses::log::pool:
    case ses::log::server:
    case ses::log::client:
    case ses::log::net:
    case ses::log::proxy:
      strm << COLOR_NC;
      break;
    case ses::log::any:
    default:
      break;
  }
}
}

void initialize(boost::log::trivial::severity_level level, bool syslog)
{
//  boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char >("Severity");

  auto trivialLogger = ::boost::log::trivial::logger::get();
  auto logger = boost::log::core::get();

  logger->reset_filter();
  logger->remove_all_sinks();

//  boost::log::add_common_attributes();
  logger->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
  logger->add_global_attribute("ThreadID", boost::log::attributes::current_thread_id());
//    boost::log::aux::default_attribute_names::timestamp(), boost::log::attributes::local_clock());
//  boost::log::core::get()->add_global_attribute("ThreadID", boost::log::attributes::current_thread_id());

//  auto sink = boost::log::add_console_log();
//  sink->set_formatter(&formatter);

//  auto fileBackend =
//      boost::make_shared<boost::log::sinks::text_file_backend>(
//          boost::log::keywords::file_name = "ses_proxy_%5N.log",
//          boost::log::keywords::rotation_size = 5 * 1024 * 1024,
//          boost::log::keywords::time_based_rotation =
//              boost::log::sinks::file::rotation_at_time_point(12, 0, 0)
//      );
//  auto fileSink =
//      boost::make_shared<boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend> >(fileBackend);
//  logger->add_sink(fileSink);


  if (!syslog)
  {
    boost::log::add_console_log(
      std::cout,
      boost::log::keywords::format =
        (
          boost::log::expressions::stream
            << "["
            << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
            << "] "
            << "[" << boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
            << "] "
            << boost::log::expressions::wrap_formatter(&formatMessagePart)
        )
    );
  }
  else
  {
    // Create a syslog sink
    auto sinkBackend = boost::make_shared<boost::log::sinks::syslog_backend>(
      //boost::log::keywords::facility = boost::log::sinks::syslog::user,
      boost::log::keywords::use_impl = boost::log::sinks::syslog::native
    );

    boost::log::sinks::syslog::custom_severity_mapping<boost::log::trivial::severity_level > mapping("Severity");
    mapping[boost::log::trivial::trace] = boost::log::sinks::syslog::debug;
    mapping[boost::log::trivial::debug] = boost::log::sinks::syslog::info;
    mapping[boost::log::trivial::info] = boost::log::sinks::syslog::notice;
    mapping[boost::log::trivial::warning] = boost::log::sinks::syslog::warning;
    mapping[boost::log::trivial::error] = boost::log::sinks::syslog::error;
    mapping[boost::log::trivial::fatal] = boost::log::sinks::syslog::critical;
    sinkBackend->set_severity_mapper(mapping);

    sinkBackend->set_target_address("localhost");

    auto sink = boost::make_shared<boost::log::sinks::synchronous_sink<boost::log::sinks::syslog_backend> >(sinkBackend);
    sink->set_formatter
      (
        boost::log::expressions::stream
          << "[" << boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
          << "] "
          << boost::log::expressions::wrap_formatter(&formatMessagePart)
      );

    logger->add_sink(sink);
  }

  logger->set_filter(boost::log::trivial::severity >= level);


}

std::string currentExceptionDiagnosticInformation()
{
  return boost::current_exception_diagnostic_information();
}
} // namespace log
} // namespace ses

std::ostream& operator<<(std::ostream& strm, ::ses::log::Component component)
{
  const auto componentString = ses::log::to_string(component);
  if (componentString != nullptr)
  {
    strm << componentString;
  }
  return strm;
}
