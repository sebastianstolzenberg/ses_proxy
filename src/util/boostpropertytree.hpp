#ifndef SES_UTIL_BOOSTPROPERTYTREE_HPP
#define SES_UTIL_BOOSTPROPERTYTREE_HPP

namespace ses {
namespace util {
namespace boostpropertytree {

inline boost::property_tree::ptree stringToPtree(const std::string& string)
{
  std::istringstream stream(string);
  boost::property_tree::ptree tree;
  boost::property_tree::read_json(stream, tree);
  return tree;
}

inline std::string ptreeToString(const boost::property_tree::ptree& ptree, bool pretty = true)
{
  std::ostringstream stream;
  boost::property_tree::write_json(stream, ptree, pretty);
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

#endif //SES_UTIL_BOOSTPROPERTYTREE_HPP
