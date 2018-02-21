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
  pt::ptree tree = util::boostpropertytree::stringToPtree(params);
  std::string login = tree.get<std::string>("login", "");
  std::string pass = tree.get<std::string>("pass", "");
  std::string agent = tree.get<std::string>("agent", "");
  handler(jsonRequestId, login, pass, agent);
}

void parseSubmit(const std::string& jsonRequestId, const std::string& params, SubmitHandler& handler)
{
  pt::ptree tree = util::boostpropertytree::stringToPtree(params);
  std::string identifier = tree.get<std::string>("id", "");
  std::string jobIdentifier = tree.get<std::string>("job_id", "");
  std::string nonce = tree.get<std::string>("nonce", "");
  std::string result = tree.get<std::string>("result", "");
  handler(jsonRequestId, identifier, jobIdentifier, nonce, result);
}

void parseKeepaliveD(const std::string& jsonRequestId, const std::string& params, KeepAliveDHandler& handler)
{
  pt::ptree tree = util::boostpropertytree::stringToPtree(params);
  std::string identifier = tree.get<std::string>("id", "");
  handler(jsonRequestId, identifier);
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

std::string createLoginResponse(const std::string& id, const std::optional<Job>& job)
{
  pt::ptree tree;
  tree.put("id", id);
  if (job)
  {
    tree.put("job.blob", job->getBlobHexString());
    tree.put("job.job_id", job->getJobId());
    tree.put("job.target", job->getTargetHexString());
    tree.put("job.id", job->getId());
  }
  tree.put("status", "OK");
  return util::boostpropertytree::ptreeToString(tree);
}

std::string createJobNotification(const Job& job)
{
  pt::ptree tree;
  tree.put("blob", job.getBlobHexString());
  tree.put("job_id", job.getJobId());
  tree.put("target", job.getTargetHexString());
  tree.put("id", job.getId());
  return util::boostpropertytree::ptreeToString(tree);
}

} // namespace server

namespace client {

namespace {
void parseError(const std::string& error, ErrorHandler& handler)
{
  pt::ptree tree = util::boostpropertytree::stringToPtree(error);
  int code = tree.get<int>("code", -1);
  std::string message = tree.get<std::string>("message", "");
  handler(code, message);
}

Job::Ptr parseJob(const pt::ptree& tree)
{
  return std::make_shared<Job>(tree.get<std::string>("blob", ""), tree.get<std::string>("job_id", ""),
                               tree.get<std::string>("target", ""), tree.get<std::string>("id", ""));
}

}

std::string createLoginRequest(const std::string& login, const std::string& pass, const std::string& agent)
{
  pt::ptree tree;
  tree.put("login", login);
  tree.put("pass", pass);
  tree.put("agent", agent);
  return util::boostpropertytree::ptreeToString(tree);
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
    pt::ptree tree = util::boostpropertytree::stringToPtree(result);
    std::string identifier = tree.get<std::string>("id", "");
    Job::Ptr optionalJob;
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
    pt::ptree tree = util::boostpropertytree::stringToPtree(result);
    successHandler(parseJob(tree));
  }
}

std::string createSubmitRequest(const std::string& id, const std::string& jobId,
                                const std::string& nonce, const std::string& result)
{
  pt::ptree tree;
  tree.put("id", id);
  tree.put("job_id", jobId);
  tree.put("nonce", nonce);
  tree.put("result", result);
  return util::boostpropertytree::ptreeToString(tree);
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
    pt::ptree tree = util::boostpropertytree::stringToPtree(result);
    std::string status = tree.get<std::string>("status", "");
    successHandler(status);
  }
}

void parseNotification(const std::string& method, const std::string& params, NewJobHandler newJobHandler)
{
  if (method == "job")
  {
    pt::ptree tree = util::boostpropertytree::stringToPtree(params);
    newJobHandler(parseJob(tree));
  }
}

} // namespace client

} // namespace stratum
} // namespace ses