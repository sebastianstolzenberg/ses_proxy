#include <iostream>
#include <memory>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/program_options.hpp>

#include "proxy/proxy.hpp"
#include "proxy/configurationfile.hpp"
#include "util/log.hpp"

struct Parameters
{
  std::string configurationFilePath_;
  bool deamon_;
};

Parameters parseCommandLine(int argc,  char** argv)
{
  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("config,c", po::value<std::string>()->default_value("config.json"), "Configuration file")
    ("thread,t", po::value<uint32_t>()->default_value(0), "Number of threads to be utilized\n (0 - automatic selection)")
    ("log-level,l", po::value<uint32_t>()->default_value(4), "Log level\n (0 - off, 1 - fatal, 2 - error, 3 - info, 4 - debug, 5 - trace)");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    throw 1;
  }

  po::notify(vm);

  Parameters parameters;
  if (vm.count("config")) {
     parameters.configurationFilePath_ = vm["config"].as<std::string>();
  }
  if (vm.count("daemon")) {
    parameters.deamon_ = vm["daemon"].as<bool>();
  }
  return parameters;
}

int main(int argc,  char** argv)
{
  std::cout << std::endl << std::endl << "Starting proxy." << std::endl << std::endl;

  std::shared_ptr<boost::asio::io_service> ioService = std::make_shared<boost::asio::io_service>();

  Parameters parameters;
  try
  {
    parameters = parseCommandLine(argc, argv);
  }
  catch (int e)
  {
    return e;
  }
  catch (...)
  {
    return -1;
  }

  ses::proxy::Proxy::Ptr proxy = std::make_shared<ses::proxy::Proxy>(ioService,
                                                                     parameters.configurationFilePath_);
  proxy->reloadConfiguration();
  proxy->run();

  std::cout << std::endl << std::endl << "Proxy terminated." << std::endl << std::endl;
  return 0;
}