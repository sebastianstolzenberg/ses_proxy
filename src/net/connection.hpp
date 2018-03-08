#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>

#include "net/connectiontype.hpp"

namespace ses {
namespace net {

class Connection : private boost::noncopyable
{
public:
  typedef std::shared_ptr<Connection> Ptr;
  typedef std::function<void(char* data, std::size_t size)> ReceivedDataHandler;
  typedef std::function<void(const std::string& error)> ErrorHandler;

protected:
  Connection() = default;
  Connection(const Connection::ReceivedDataHandler& receivedDataHandler,
             const Connection::ErrorHandler& errorHandler);
  virtual ~Connection() = default;

  void notifyRead(char* data, size_t size);
  void notifyError(const std::string& error);

public:
  void setHandler(const ReceivedDataHandler& receivedDataHandler, const ErrorHandler& errorHandler);
  void resetHandler();

  virtual bool isConnected() const = 0;
  virtual std::string getConnectedIp() const = 0;
  virtual uint16_t getConnectedPort() const = 0;

  virtual bool send(const char* data, std::size_t size) = 0;
  bool send(const std::string& data) {return send(data.data(), data.size());}

protected:
  virtual void startReading() = 0;

private:
  ReceivedDataHandler receivedDataHandler_;
  ErrorHandler errorHandler_;
};

} //namespace net
} //namespace ses
