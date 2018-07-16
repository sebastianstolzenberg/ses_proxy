#include <iostream>
#include <iomanip>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "util/shell.hpp"

namespace ses {
namespace util {
namespace shell {

Command::Command(const std::string& command, Handler handler, const std::string& description)
  : command_(command), handler_(handler), description_(description)
{
}

const std::string& Command::getCommand() const
{
  return command_;
}

const std::string& Command::getDescription() const
{
  return description_;
}

bool Command::operator==(const std::string& otherCommand)
{
  return command_ == otherCommand;
}

void Command::operator()()
{
  operator()(std::vector<std::string>());
}

void Command::operator()(const std::vector<std::string>& parameter)
{
  if (handler_)
  {
    handler_(parameter);
  }
}

Shell::Shell(const std::shared_ptr<boost::asio::io_service>& ioService)
  : ioService_(ioService), userOutput_(*ioService, ::dup(STDOUT_FILENO)), userInput_(*ioService, ::dup(STDIN_FILENO))
    , userInputBuffer_(1000)
{
  // add default commands
  addCommand(Command("help", std::bind(&Shell::listCommands, this), " List commands"));
}

void Shell::addCommand(const Command& command)
{
  commands_[command.getCommand()] = command;
}

void Shell::removeCommand(const std::string& command)
{
  commands_.erase(command);
}

void Shell::start()
{
  startWaitingForNextUserInput();
}

void Shell::stop()
{
  userInput_.close();
  userOutput_.close();
}

void Shell::listCommands()
{
  std::cout << "Allowed commands:" << std::endl;
  for (auto& command : commands_)
  {
    std::cout << " " << std::left << std::setw(10) << command.second.getCommand()
              << std::setw(1) << " : " << command.second.getDescription() << std::endl;
  }
}

Command& Shell::fetchCommand(const std::string& commandString)
{
  auto currentIt = commands_.begin();
  auto endIt = commands_.end();
  auto foundIt = endIt;
  for (;currentIt != endIt; ++currentIt)
  {
    if (boost::starts_with(currentIt->first, commandString))
    {
      if (foundIt == endIt)
      {
        foundIt = currentIt;
      }
      else
      {
        // second possible match found, aborts search
        foundIt = endIt;
        break;
      }
    }
  }
  if (foundIt != endIt)
  {
    return foundIt->second;
  }
  else
  {
    std::cout << "Error! No command found matching \"" << commandString << "\"" << std::endl << std::endl;
    return commands_["help"];
  }
}

void Shell::startWaitingForNextUserInput()
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

void Shell::processInput(const std::string& input)
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
  fetchCommand(command)(parameters);
}


} // namespace shell
} // namespace util
} // namespace ses
