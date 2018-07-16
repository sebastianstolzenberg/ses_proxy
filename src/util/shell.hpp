#pragma once

#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

#include <boost/asio.hpp>


namespace ses {
namespace util {
namespace shell {

class Command
{
public:
  typedef std::vector<std::string> Parameters;
  typedef std::function<void(const Parameters& parameter)> Handler;

public:
  Command() = default;
  Command(const std::string& command, Handler handler, const std::string& description = "");

  const std::string& getCommand() const;
  const std::string& getDescription() const;

  bool operator==(const std::string& otherCommand);
  void operator()();
  void operator()(const std::vector<std::string>& parameter);

private:
  std::string command_;
  std::string description_;
  Handler handler_;
};

class Shell : public std::enable_shared_from_this<Shell>
{
public:
  typedef std::shared_ptr<Shell> Ptr;

  Shell(const std::shared_ptr<boost::asio::io_service>& ioService);

  void addCommand(const Command& command);
  void removeCommand(const std::string& command);

  void start();
  void stop();

  void listCommands();
  Command& fetchCommand(const std::string& commandString);

private:
  void startWaitingForNextUserInput();
  void processInput(const std::string& input);

private:
  std::shared_ptr<boost::asio::io_service> ioService_;

  boost::asio::posix::stream_descriptor userOutput_;
  boost::asio::posix::stream_descriptor userInput_;
  boost::asio::streambuf userInputBuffer_;

  std::unordered_map<std::string, Command> commands_;
};

} // namespace shell
} // namespace util
} // namespace ses
