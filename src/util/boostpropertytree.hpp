#pragma once

#include <boost/exception/diagnostic_information.hpp>

namespace ses {
namespace util {
namespace boostpropertytree {

inline boost::property_tree::ptree stringToPtree(const std::string& string)
{
  std::istringstream stream(string);
  boost::property_tree::ptree tree;
  try
  {
    boost::property_tree::read_json(stream, tree);
  }
  catch (...)
  {
  }
  return tree;
}

inline std::string ptreeToString(const boost::property_tree::ptree& ptree, bool pretty = true)
{
  std::ostringstream stream;
  try
  {
    boost::property_tree::write_json(stream, ptree, pretty);
  }
  catch (...)
  {
  }
  return stream.str();
}

inline std::string ptreeToString(const boost::optional<boost::property_tree::ptree&>& ptree, bool pretty = true)
{
  std::string result;
  if (ptree)
  {
    result = ptreeToString(*ptree, pretty);
  }
  return result;
}

} // namespace boostpropertytree
} // namespace util
} // namespace ses
