
#include <iostream>

#include "util/boostpropertytree.hpp"
#include "net/jsonrpc/jsonrpc.hpp"

namespace ses {
namespace net {
namespace jsonrpc {

namespace pt = boost::property_tree;

std::string request(const std::string& id, const std::string& method, const std::string& params)
{
  std::ostringstream request;
  request << "{"
          << "\"id\":" << id << ","
          << "\"jsonrpc\":\"2.0\","
          << "\"method\":\"" << method << "\"";
  if (!params.empty())
  {
    request << ",\"params\":" << params;
  }
  request << "}\n";
  return request.str();
}

std::string notification(const std::string& method, const std::string& params)
{
  std::ostringstream notification;
  notification << "{"
               << "\"jsonrpc\":\"2.0\","
               << "\"method\":\"" << method << "\"";
  if (!params.empty())
  {
    notification << ",\"params\":" << params;
  }
  notification << "}\n";

  return notification.str();
}

std::string response(const std::string& id, const std::string& result, const std::string& error)
{
  std::ostringstream response;
  response << "{"
          << "\"id\":" << id << ","
          << "\"jsonrpc\":\"2.0\"";
  if (!result.empty())
  {
    response << ",\"result\":" << result;
  }
  if (!error.empty())
  {
    response << ",\"error\":" << error;
  }
  response << "}\n";
  return response.str();
}

std::string statusResponse(const std::string& id, const std::string& status)
{
  std::ostringstream response;
  response << "{"
           << "\"id\":" << id << ","
           << "\"jsonrpc\":\"2.0\""
           << ",\"result\":{\"status\":\"" << status << "\"}}\n";
  return response.str();
}

std::string errorResponse(const std::string& id, int code, const std::string& message)
{
  std::ostringstream response;
  response << "{"
           << "\"id\":" << id << ","
           << "\"jsonrpc\":\"2.0\""
           << ",\"error\":{"
           << "\"code\":" << code << ","
           << "\"message\":\"" << message << "\"}}\n";
  return response.str();
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
