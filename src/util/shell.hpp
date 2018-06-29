#pragma once

#include <string>
#include <functional>
#include <map>

#include <boost/asio.hpp>
#include <boost/tokenizer.hpp>

namespace ses {
namespace util {

class Command
{
public:
  typedef std::function<void (const std::string& parameter)> Handler;
public:
  Command() = default;

  Command(const std::string& command, Handler& handler, const std::string& description = "")
    : command_(command), handler_(handler), description_(description)
  {
  }

  const std::string& getCommand() const
  {
    return command_;
  }

  bool operator==(const std::string& otherCommand)
  {
    return command_ == otherCommand;
  }

  void operator()(const std::string& parameter)
  {
    if (handler_)
    {
      handler_(parameter);
    }
  }

private:
  std::string command_;
  std::string description_;
  Handler handler_;
};

class Shell
{
public:

  Shell(const std::shared_ptr<boost::asio::io_service>& ioService)
    : userOutput_(*ioService, ::dup(STDOUT_FILENO)),
      userInput_(*ioService, ::dup(STDIN_FILENO)),
      userInputBuffer_(1000)
  {

  }

  void addCommand(Command& command)
  {
    commands_[command.getCommand()] = command;
  }

  void removeCommand(const std::string& command)
  {
    commands_.erase(command);
  }

  void Shell::startWaitingForNextUserInput()
  {
    // Read a line of input entered by the user.
    boost::asio::async_read_until(userInput_, userInputBuffer_, '\n',
                                  boost::bind(&Shell::handleUserInput, this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
  }

  void Shell::handleUserInput(const boost::system::error_code& error,
                              std::size_t length)
  {
    if (!error)
    {
      std::string command(extractString(userInputBuffer_.data(), length));
      // TODO: fix zeroization for NIAP testing/demonstations. The input buffer
      // needs to be wiped, because it may contain the SIP password when entering
      // it. The code which is commented out sometimes corrupts the input buffer
      // handling. This causes that no further commands can be entered. An proper
      // solution is needed for this problem.
      // uint8_t* buffer = (uint8_t*) boost::asio::buffer_cast<const char*>(userInputBuffer_.data());
      // crypto_util::memsetSec(buffer - length, 0, length);
      userInputBuffer_.consume(length);

      processCommand(command);
      crypto_util::memsetSec((uint8_t*) command.data(), 0, command.size());
    }

    startWaitingForNextUserInput();
  }

  std::string Shell::readStringFromUserInput(const std::string& defaultString)
  {
    std::size_t length = boost::asio::read_until(userInput_, userInputBuffer_, '\n');
    std::string result = extractString(userInputBuffer_.data(), length);
    userInputBuffer_.consume(length);

    // drops new line character from string
    result.erase(result.end() - 1);

    if (result.empty())
    {
      result = defaultString;
    }

    return result;
  }

private:
  std::map<std::string, Command> commands_;

  boost::asio::posix::stream_descriptor userOutput_;
  boost::asio::posix::stream_descriptor userInput_;
  boost::asio::streambuf userInputBuffer_;
};

} // namespace util
} // namespace ses
