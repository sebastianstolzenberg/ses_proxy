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
  typedef std::weak_ptr<Connection> WeakPtr;
  typedef std::function<void()> ConnectHandler;
  typedef std::function<void(const std::string& data)> ReceivedDataHandler;
  typedef std::function<void(const std::string& error)> DisconnectHandler;

protected:
  Connection() = default;
  Connection(const Connection::ReceivedDataHandler& receivedDataHandler,
             const Connection::DisconnectHandler& errorHandler);
  virtual ~Connection() = default;

  void notifyRead(const std::string& data);
  void notifyError(const std::string& error);

public:
  void setHandler(const ReceivedDataHandler& receivedDataHandler, const DisconnectHandler& errorHandler);

  void resetHandler();

  virtual void setSelfSustainingUntilDisconnect(bool selfSustain) = 0;
  virtual void disconnect() = 0;
  virtual bool isConnected() const = 0;
  virtual std::string getConnectedIp() const = 0;
  virtual uint16_t getConnectedPort() const = 0;

  virtual void send(const std::string& data) = 0;

protected:
  virtual void startReading() = 0;

private:
  ReceivedDataHandler receivedDataHandler_;
  DisconnectHandler errorHandler_;
};

} //namespace net
} //namespace ses
