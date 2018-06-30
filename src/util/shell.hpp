#pragma once

#include <iostream>
#include <memory>
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
  typedef std::function<void (const std::vector<std::string>& parameter)> Handler;

public:
  Command() = default;

  Command(const std::string& command, Handler handler, const std::string& description = "")
    : command_(command), handler_(handler), description_(description)
  {
  }

  const std::string& getCommand() const
  {
    return command_;
  }

  const std::string& getDescription() const
  {
    return description_;
  }

  bool operator==(const std::string& otherCommand)
  {
    return command_ == otherCommand;
  }

  void operator()(const std::vector<std::string>& parameter)
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

class Shell : public std::enable_shared_from_this<Shell>
{
public:
  typedef std::shared_ptr<Shell> Ptr;

  Shell(const std::shared_ptr<boost::asio::io_service>& ioService)
    : ioService_(ioService), userOutput_(*ioService, ::dup(STDOUT_FILENO)), userInput_(*ioService, ::dup(STDIN_FILENO)),
      userInputBuffer_(1000)
  {
    // add default commands
    addCommand(Command("h", std::bind(&Shell::listCommands, this), " List commands"));
  }

  void addCommand(const Command& command)
  {
    commands_[command.getCommand()] = command;
  }

  void removeCommand(const std::string& command)
  {
    commands_.erase(command);
  }

  void start()
  {
    startWaitingForNextUserInput();
  }

  void stop()
  {
    userInput_.close();
    userOutput_.close();
  }

  void listCommands()
  {
    for (auto& command : commands_)
    {
      std::cout << command.second.getCommand() << " : " << command.second.getDescription() << std::endl;
    }
  }

private:
  void startWaitingForNextUserInput()
  {
    // Read a line of input entered by the user.
    auto self = shared_from_this();
    boost::asio::async_read_until(
      userInput_, userInputBuffer_, '\n',
      [self, this](const boost::system::error_code& error, std::size_t length)
      {
        if (!error)
        {
          std::string input(boost::asio::buffers_begin(userInputBuffer_.data()),
                            boost::asio::buffers_begin(userInputBuffer_.data()) + length);
          userInputBuffer_.consume(length);

          processInput(input);
          startWaitingForNextUserInput();
        }
      });
  }

  void processInput(const std::string& input)
  {
    boost::char_separator<char> separator(" \n");
    boost::tokenizer<boost::char_separator<char> > tokens(input, separator);
    std::string command;
    std::vector<std::string> parameters;
    for (const auto& token : tokens)
    {
      if (command.empty())
      {
        command = token;
      }
      else
      {
        parameters.push_back(token);
      }
    }
    commands_[command](parameters);
  }

private:
  std::shared_ptr<boost::asio::io_service> ioService_;

  boost::asio::posix::stream_descriptor userOutput_;
  boost::asio::posix::stream_descriptor userInput_;
  boost::asio::streambuf userInputBuffer_;

  std::map<std::string, Command> commands_;
};

} // namespace util
} // namespace ses
