
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "util/boostpropertytree.hpp"
#include "net/jsonrpc/jsonrpc.hpp"

namespace ses {
namespace net {
namespace jsonrpc {

namespace pt = boost::property_tree;

std::string request(const std::string& id, const std::string& method, const std::string& params)
{
  pt::ptree requestTree;
  requestTree.put("id", id);
  requestTree.put("jsonrpc", "2.0");
  requestTree.put("method", method);

  if (!params.empty())
  {
    requestTree.put_child("params", util::boostpropertytree::stringToPtree(params));
  }

  return util::boostpropertytree::ptreeToString(requestTree, false);
}

std::string notification(const std::string& method, const std::string& params)
{
  pt::ptree notificationTree;
  notificationTree.put("method", method);
  notificationTree.put("jsonrpc", "2.0");

  if (!params.empty())
  {
    notificationTree.put_child("params", util::boostpropertytree::stringToPtree(params));
  }

  return util::boostpropertytree::ptreeToString(notificationTree, false);
}

std::string response(const std::string& id, const std::string& result, const std::string& error)
{
  pt::ptree responseTree;
  responseTree.put("id", id);
  responseTree.put("jsonrpc", "2.0");

  if (!result.empty())
  {
    responseTree.put_child("result", util::boostpropertytree::stringToPtree(result));
  }

  if (!error.empty())
  {
    responseTree.put_child("error", util::boostpropertytree::stringToPtree(error));
  }

  return util::boostpropertytree::ptreeToString(responseTree, false);
}

std::string statusResponse(const std::string& id, const std::string& status)
{
  pt::ptree responseTree;
  responseTree.put("id", id);
  responseTree.put("jsonrpc", "2.0");
  responseTree.put("result.status", status);
  return util::boostpropertytree::ptreeToString(responseTree, false);
}

std::string errorResponse(const std::string& id, int code, const std::string& message)
{
  pt::ptree responseTree;
  responseTree.put("id", id);
  responseTree.put("jsonrpc", "2.0");
  responseTree.put("error.code", code);
  responseTree.put("error.message", message);
  return util::boostpropertytree::ptreeToString(responseTree, false);
}

bool parse(const std::string& jsonrpc,
           std::function<void (const std::string& id, const std::string& method, const std::string& params)> requestHandler,
           std::function<void (const std::string& id, const std::string& result, const std::string& error)> responseHandler,
           std::function<void (const std::string& method, const std::string& params)> notificatonHandler)
{
  pt::ptree jsonrpcTree = util::boostpropertytree::stringToPtree(jsonrpc);

  std::string id = jsonrpcTree.get<std::string>("id", "null");
//  std::string jsonrpcVersion = jsonrpcTree.get<std::string>("jsonrpc", "");
  std::string method = jsonrpcTree.get<std::string>("method", "");

  std::string params = util::boostpropertytree::ptreeToString(jsonrpcTree.get_child_optional("params"));
  std::string result = util::boostpropertytree::ptreeToString(jsonrpcTree.get_child_optional("result"));
  std::string error = util::boostpropertytree::ptreeToString(jsonrpcTree.get_child_optional("error"));

  bool success = false;
  if (id == "null")
  {
    if (!method.empty())
    {
      notificatonHandler(method, params);
      success = true;
    }
  }
  else
  {
    if (!method.empty())
    {
      requestHandler(id, method, params);
      success = true;
    }
    else if (!result.empty() || !error.empty())
    {
      responseHandler(id, result, error);
      success = true;
    }
  }
  return success;
}

} // namespace jsonrpc
} // namespace net
} // namespace ses
