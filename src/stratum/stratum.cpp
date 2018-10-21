#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "util/boostpropertytree.hpp"
#include "stratum/stratum.hpp"

namespace ses {
namespace stratum {
namespace pt = boost::property_tree;

namespace server {
namespace {
void parseLogin(const std::string& jsonRequestId, const std::string& params, LoginHandler& handler)
{
  auto tree = util::boostpropertytree::stringToPtree(params);
  auto login = tree.get<std::string>("login", "");
  auto pass = tree.get<std::string>("pass", "");
  auto agent = tree.get<std::string>("agent", "");
  auto algorithm = tree.get<std::string>("algo", "");
  auto algorithmVariantsNode = tree.get_child_optional("supported-variants");
  std::vector<std::string> algorithmVariants;
  if (algorithmVariantsNode)
  {
    algorithmVariants.reserve(algorithmVariantsNode->size());
    for (auto algorithmNode : *algorithmVariantsNode)
    {
      auto algorithmString = algorithmNode.second.get_value<std::string>("");
      algorithmVariants.push_back(algorithmString);
    }
  }
  handler(jsonRequestId, login, pass, agent, algorithm, algorithmVariants);
}

void parseSubmit(const std::string& jsonRequestId, const std::string& params, SubmitHandler& handler)
{
  auto tree = util::boostpropertytree::stringToPtree(params);
  auto identifier = tree.get<std::string>("id", "");
  auto jobIdentifier = tree.get<std::string>("job_id", "");
  auto nonce = tree.get<std::string>("nonce", "");
  auto result = tree.get<std::string>("result", "");
  auto workerNonce = tree.get<std::string>("workerNonce", "");
  auto poolNonce = tree.get<std::string>("poolNonce", "");
  handler(jsonRequestId, identifier, jobIdentifier, nonce, result, workerNonce, poolNonce);
}

void parseKeepaliveD(const std::string& jsonRequestId, const std::string& params, KeepAliveDHandler& handler)
{
  auto tree = util::boostpropertytree::stringToPtree(params);
  auto identifier = tree.get<std::string>("id", "");
  handler(jsonRequestId, identifier);
}

std::string getJobTree(const Job& job)
{
  std::ostringstream jobTree;
  jobTree << "{"
          << "\"id\":\"" << job.getId() << "\","
          << "\"job_id\":\"" << job.getJobIdentifier() << "\","
          << "\"algo\":\"" << job.getAlgo() << "\","
          << "\"variant\":\"" << job.getVariant() << "\",";
  if (job.isBlockTemplate())
  {
    jobTree << "\"blocktemplate_blob\":\"" << job.getBlocktemplateBlob() << "\","
            << "\"difficulty\":" << job.getDifficulty() << ","
            << "\"height\":" << job.getHeight() << ","
            << "\"reserved_offset\":" << job.getReservedOffset() << ","
            << "\"client_nonce_offset\":" << job.getClientNonceOffset() << ","
            << "\"client_pool_offset\":" << job.getClientPoolOffset() << ","
            << "\"target_diff\":" << job.getTargetDiff();
  }
  else
  {
    jobTree << "\"blob\":\"" << job.getBlob() << "\","
            << "\"target\":\"" << job.getTarget() << "\"";
  }
  jobTree << "}";
  return jobTree.str();
}
}

void parseRequest(const std::string& jsonRequestId, const std::string& method, const std::string& params,
                  LoginHandler loginHandler, GetJobHandler getJobHandler, SubmitHandler submitHandler,
                  KeepAliveDHandler keepAliveDHandler, UnknownMethodHandler unknownMethodHandler)
{
  if (method == "login")
  {
    parseLogin(jsonRequestId, params, loginHandler);
  }
  else if (method == "getjob")
  {
    getJobHandler(jsonRequestId);
  }
  else if (method == "submit")
  {
    parseSubmit(jsonRequestId, params, submitHandler);
  }
  else if (method == "keepalived")
  {
    parseKeepaliveD(jsonRequestId, params, keepAliveDHandler);
  }
  else
  {
    unknownMethodHandler(jsonRequestId);
  }
}

std::string createLoginResponse(const std::string& id, const boost::optional<Job>& job)
{
  std::ostringstream response;
  response << "{\"id\":\"" << id << "\",";
  if (job)
  {
    response << "\"job\":" << getJobTree(*job) << ",";
  }
  response << "\"status\":\"OK\"}";
  return response.str();
}

std::string createJobNotification(const Job& job)
{
  return getJobTree(job);
}

} // namespace server

namespace client {

namespace {
void parseError(const std::string& error, ErrorHandler& handler)
{
  auto tree = util::boostpropertytree::stringToPtree(error);
  auto code = tree.get<int>("code", -1);
  auto message = tree.get<std::string>("message", "");
  handler(code, message);
}

Job parseJob(const pt::ptree& tree)
{
  return Job(tree.get<std::string>("id", ""), tree.get<std::string>("job_id", ""),
             tree.get<std::string>("blob", ""), tree.get<std::string>("target", ""),
             tree.get<std::string>("blocktemplate_blob", ""), tree.get<std::string>("difficulty", ""),
             tree.get<std::string>("height", ""), tree.get<std::string>("reserved_offset", ""),
             tree.get<std::string>("client_nonce_offset", ""), tree.get<std::string>("client_pool_offset", ""),
             tree.get<std::string>("target_diff", ""), tree.get<std::string>("target_diff_hex", ""),
             tree.get<std::string>("job_id", ""));
}

}

std::string createLoginRequest(const std::string& login, const std::string& pass, const std::string& agent)
{
  std::ostringstream request;
  request << "{"
          << "\"login\":\"" << login << "\","
          << "\"pass\":\"" << pass << "\","
          << "\"agent\":\"" << agent << "\""
          << "}";
  return request.str();
}

void parseLoginResponse(const std::string& result, const std::string& error,
                        LoginSuccessHandler successHandler, ErrorHandler errorHandler)
{
  if (!error.empty())
  {
    parseError(error, errorHandler);
  }
  else
  {
    auto tree = util::boostpropertytree::stringToPtree(result);
    auto identifier = tree.get<std::string>("id", "");
    boost::optional<Job> optionalJob;
    if (tree.count("job") > 0)
    {
      optionalJob = parseJob(tree.get_child("job"));
    }
    successHandler(identifier, optionalJob);
  }
}

void parseGetJobResponse(const std::string& result, const std::string& error,
                         GetJobSuccessHandler successHandler, ErrorHandler errorHandler)
{
  if (!error.empty())
  {
    parseError(error, errorHandler);
  }
  else
  {
    auto tree = util::boostpropertytree::stringToPtree(result);
    successHandler(parseJob(tree));
  }
}

std::string createSubmitParams(const std::string& id, const std::string& jobId,
                               const std::string& nonce, const std::string& result,
                               boost::optional<uint32_t> workerNonce,
                               boost::optional<uint32_t> poolNonce)
{
  std::ostringstream request;
  request << "{"
          << "\"id\":\"" << id << "\","
          << "\"job_id\":\"" << jobId << "\","
          << "\"nonce\":\"" << nonce << "\","
          << "\"result\":\"" << result << "\"";
  if (workerNonce)
  {
    request << ",\"workerNonce\":" << *workerNonce;
  }
  if (poolNonce)
  {
    request << ",\"poolNonce\":" << *poolNonce;
  }
  request << "}";
  return request.str();
}

void parseSubmitResponse(const std::string& result, const std::string& error,
                         SubmitSuccessHandler successHandler, ErrorHandler errorHandler)
{
  if (!error.empty())
  {
    parseError(error, errorHandler);
  }
  else
  {
    auto tree = util::boostpropertytree::stringToPtree(result);
    auto status = tree.get<std::string>("status", "");
    successHandler(status);
  }
}

std::string createKeepalivedParams(const std::string& id)
{
  std::ostringstream request;
  request << "{"
          << "\"id\":\"" << id << "\""
          << "}";
  return request.str();
}

void parseNotification(const std::string& method, const std::string& params, NewJobHandler newJobHandler)
{
  if (method == "job")
  {
    auto tree = util::boostpropertytree::stringToPtree(params);
    newJobHandler(parseJob(tree));
  }
}

} // namespace client

} // namespace stratum
} // namespace ses
