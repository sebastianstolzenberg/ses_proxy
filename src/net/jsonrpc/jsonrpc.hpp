#ifndef SES_NET_JSONRPC_JSONRPC_HPP
#define SES_NET_JSONRPC_JSONRPC_HPP

#include <string>
#include <functional>

namespace ses {
namespace net {
namespace jsonrpc {

std::string request(const std::string& id, const std::string& method, const std::string& parameters);

std::string notification(const std::string& method, const std::string& parameters);

std::string response(const std::string& id, const std::string& result, const std::string& error);

std::string statusResponse(const std::string& id, const std::string& status);
std::string errorResponse(const std::string& id, int code, const std::string& message);

bool parse(const std::string& jsonrpc,
           std::function<void (const std::string& id, const std::string& method, const std::string& params)> requestHandler,
           std::function<void (const std::string& id, const std::string& result, const std::string& error)> responseHandler,
           std::function<void (const std::string& method, const std::string& params)> notificatonHandler);

} // namespace jsonrpc
} // namespace net
} // namespace ses

#endif //SES_NET_JSONRPC_JSONRPC_HPP
