#ifndef SES_NET_JSONRPC_JSONRPC_HPP
#define SES_NET_JSONRPC_JSONRPC_HPP

namespace ses {
namespace net {
namespace jsonrpc {

std::string request(const std::string& id, const std::string& method, const std::string& parameters);

std::string notification(const std::string& method, const std::string& parameters);

std::string response(const std::string& id, const std::string& result, const std::string& error);

std::string okResponse(const std::string& id);

class ParserHandler
{
protected:
  virtual ~ParserHandler() {}

public:
  virtual void handleJsonRequest(const std::string& id, const std::string& method, const std::string& params) = 0;
  virtual void handleJsonResponse(const std::string& id, const std::string& result, const std::string& error) = 0;
  virtual void handleJsonNotification(const std::string& id, const std::string& method, const std::string& params) = 0;
};

bool parse(ParserHandler& handler, const std::string& jsonrpc);

} // namespace jsonrpc
} // namespace net
} // namespace ses

#endif //SES_NET_JSONRPC_JSONRPC_HPP
