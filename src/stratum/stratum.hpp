#ifndef SES_STRATUM_STRATUM_HPP
#define SES_STRATUM_STRATUM_HPP

#include <string>
#include <vector>
#include <functional>
#include <optional>

#include "stratum/job.hpp"

namespace ses {
namespace stratum {

namespace server {

typedef std::function<void(const std::string& jsonRequestId, const std::string& login,
                           const std::string& pass, const std::string& agent)> LoginHandler;
typedef std::function<void(const std::string& jsonRequestId)> GetJobHandler;
typedef std::function<void(const std::string& jsonRequestId, const std::string& identifier, const std::string& jobIdentifier,
                           const std::string& nonce, const std::string& result)> SubmitHandler;
typedef std::function<void(const std::string& jsonRequestId, const std::string& identifier)> KeepAliveDHandler;
typedef std::function<void(const std::string& jsonRequestId)> UnknownMethodHandler;

void parseRequest(const std::string& jsonRequestId, const std::string& method, const std::string& params,
                  LoginHandler loginHandler, GetJobHandler getJobHandler, SubmitHandler submitHandler,
                  KeepAliveDHandler keepAliveDHandler, UnknownMethodHandler unknownMethodHandler);

std::string createLoginResponse(const std::string& id, const std::optional<Job>& job = std::optional<Job>());
std::string createJobNotification(const Job& job);
} // namespace server


namespace client {
typedef std::function<void(int code, const std::string& message)> ErrorHandler;

std::string createLoginRequest(const std::string& login, const std::string& pass, const std::string& agent);
typedef std::function<void(const std::string& id, const std::optional<Job>& optionalJob)> LoginSuccessHandler;
void parseLoginResponse(const std::string& result, const std::string& error,
                        LoginSuccessHandler successHandler, ErrorHandler errorHandler);

typedef std::function<void(const Job& job)> GetJobSuccessHandler;
void parseGetJobResponse(const std::string& result, const std::string& error,
                         GetJobSuccessHandler successHandler, ErrorHandler errorHandler);

std::string createSubmitRequest(const std::string& id, const std::string& jobId,
                                const std::string& nonce, const std::string& result);
typedef std::function<void(const std::string& status)> SubmitSuccessHandler;
void parseSubmitResponse(const std::string& result, const std::string& error,
                         SubmitSuccessHandler successHandler, ErrorHandler errorHandler);

typedef std::function<void(const Job& job)> NewJobHandler;
void parseNotification(const std::string& method, const std::string& params, NewJobHandler newJobHandler);
} // namespace client

} // namespace stratum
} // namespace ses

#endif //SES_STRATUM_STRATUM_HPP
