#include <memory>
#include <string>
#include <boost/asio.hpp>

#include "net/client/connection.hpp"
#include "net/endpoint.hpp"

namespace ses {
namespace net {
namespace client {

class Http : public std::enable_shared_from_this<Http>
{
public:
  Http(const std::shared_ptr<boost::asio::io_service>& ioService,
       const EndPoint& endPoint)
    : ioService_(ioService), endPoint_(endPoint)
  {

  }

  void connect()
  {
    connection_ = net::client::establishConnection(ioService_,
                                                   endPoint_,
                                                   std::bind(&Http::handleConnect, this),
                                                   std::bind(&Http::handleReceived, this,
                                                             std::placeholders::_1),
                                                   std::bind(&Http::handleDisconnect, this,
                                                             std::placeholders::_1));

  }

private:
  void handleConnect()
  {

  }

  void handleReceived(const std::string& data)
  {

  }

  void handleDisconnect(const std::string& error)
  {

  }



  void handle_read_status_line(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Check that response is OK.
      std::istream response_stream(&response_);
      std::string http_version;
      response_stream >> http_version;
      unsigned int status_code;
      response_stream >> status_code;
      std::string status_message;
      std::getline(response_stream, status_message);
      if (!response_stream || http_version.substr(0, 5) != "HTTP/")
      {
        std::cout << "Invalid response\n";
        return;
      }
      if (status_code != 200)
      {
        std::cout << "Response returned with status code ";
        std::cout << status_code << "\n";
        return;
      }

      // Read the response headers, which are terminated by a blank line.
      boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
          boost::bind(&client::handle_read_headers, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_read_headers(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Process the response headers.
      std::istream response_stream(&response_);
      std::string header;
      while (std::getline(response_stream, header) && header != "\r")
        std::cout << header << "\n";
      std::cout << "\n";

      // Write whatever content we already have to output.
      if (response_.size() > 0)
        std::cout << &response_;

      // Start reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&client::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_read_content(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Write all of the data that has been read so far.
      std::cout << &response_;

      // Continue reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&client::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else if (err != boost::asio::error::eof)
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  std::shared_ptr<boost::asio::io_service> ioService_;
  net::EndPoint endPoint_;
  Connection::Ptr connection_;
};

} //namespace client
} //namespace net
} //namespace ses