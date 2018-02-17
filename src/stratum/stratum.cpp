#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "util/boostpropertytree.hpp"
#include "stratum/stratum.hpp"

namespace ses {
namespace stratum {

namespace pt = boost::property_tree;

namespace {
void parseServerLogin(const std::string& jsonRequestId, ServerMethodHandler& handler, const std::string& params)
{
  pt::ptree tree = util::boostpropertytree::stringToPtree(params);
  std::string login = tree.get<std::string>("login", "");
  std::string pass = tree.get<std::string>("pass", "");
  std::string agent = tree.get<std::string>("agent", "");
  handler.handleStratumServerLogin(jsonRequestId, login, pass, agent);
}

//"\"id\":\"%s\","
//"\"job_id\":\"%s\","
//"\"nonce\":\"%s\","
//"\"result\":\"%s\""

void parseServerSubmit(const std::string& jsonRequestId, ServerMethodHandler& handler, const std::string& params)
{
  pt::ptree tree = util::boostpropertytree::stringToPtree(params);
  std::string identifier = tree.get<std::string>("id", "");
  std::string jobIdentifier = tree.get<std::string>("job_id", "");
  std::string nonce = tree.get<std::string>("nonce", "");
  std::string result = tree.get<std::string>("result", "");
  handler.handleStratumServerSubmit(jsonRequestId, identifier, jobIdentifier, nonce, result);
}

}

void parseServerMethod(const std::string& jsonRequestId, ServerMethodHandler& handler,
                       const std::string& method, const std::string& params)
{
  if (method == "login")
  {
    parseServerLogin(jsonRequestId, handler, params);
  }
  else if (method == "getjob")
  {
    handler.handleStratumServerGetJob(jsonRequestId);
  }
  else if (method == "submit")
  {
    parseServerSubmit(jsonRequestId, handler, params);
  }
  else if (method == "keepalived")
  {
    handler.handleStratumServerKeepAliveD(jsonRequestId);
  }
  else if (method == "mining.authorize")
  {
    handler.handleStratumServerMiningAuthorize("", "");
  }
  else if (method == "mining.capabilities")
  {
    handler.handleStratumServerMiningCapabilities("");
  }
  else if (method == "mining.extranonce.subscribe")
  {
    handler.handleStratumMServeriningExtraNonceSubscribe();
  }
  else if (method == "mining.get_transactions")
  {
    handler.handleStratumServerMiningGetTransactions("");
  }
  else if (method == "mining.submit")
  {
    handler.handleStratumServerMiningSubmit("", "", "", "", "");
  }
  else if (method == "mining.subscribe")
  {
    handler.handleStratumServerMiningSubscribe("", "");
  }
  else if (method == "mining.suggest_difficulty")
  {
    handler.handleStratumServerMiningSuggestDifficulty("");
  }
  else if (method == "mining.suggest_target")
  {
    handler.handleStratumServerMiningSuggestTarget("");
  }
}

void parseClientMethod(const std::string& jsonRequestId, ClientMethodHandler& handler,
                       const std::string& method, const std::string& params)
{
  if (method == "client.get_version")
  {
    handler.handleStratumClientGetVersion();
  }
  else if (method == "client.reconnect")
  {
    handler.handleStratumClientReconnect("", "", "");
  }
  else if (method == "client.show_message")
  {
    handler.handleStratumClientShowMessage("");
  }
  else if (method == "mining.notify")
  {
    handler.handleStratumClientMiningNotify("", "", "", "", "", "", "", "", false);
  }
  else if (method == "mining.set_difficulty")
  {
    handler.handleStratumClientMiningSetDifficulty("");
  }
  else if (method == "mining.set_extranonce")
  {
    handler.handleStratumClientMiningSetExtraNonce("", 1);
  }
  else if (method == "mining.set_goal")
  {
    handler.handleStratumClientMiningSetGoal("");
  }
}

std::string createLoginResponse(const std::string& id,
                                const Job& job,
                                const std::string& status)
{
  pt::ptree tree;
  tree.put("id", id);
  tree.put("job.blob", job.blob_);
  tree.put("job.job_id", job.jobId_);
  tree.put("job.target", job.target_);
  tree.put("job.id", job.id_);
  tree.put("status", status);
  return util::boostpropertytree::ptreeToString(tree);
}

std::string createJobNotification(const Job& job)
{
  pt::ptree tree;
  tree.put("blob", job.blob_);
  tree.put("job_id", job.jobId_);
  tree.put("target", job.target_);
  tree.put("id", job.id_);
  return util::boostpropertytree::ptreeToString(tree);
}

std::string createSubmitResponse(bool ok)
{

}

} // namespace stratum
} // namespace ses