#ifndef SESSION_DEFINED
#define SESSION_DEFINED

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <vector>
#include <map>
#include <memory>
#include <mutex>

#include "handler.h"
#include "http_response.h"

// for tcp type
using boost::asio::ip::tcp;

class session
{
public:
  session(boost::asio::io_service& io_service,
    std::map <std::string, std::pair<std::string, std::shared_ptr<RequestHandler>>> handler_map);
  tcp::socket& socket();
  void start();
  // handles requests by seeing which type of response is required
  int handle_request();
  // writes out a string to the socket and closes the connection
  void write_string(std::string send);

private:
  std::string get_message_request();
  std::string get_longest_prefix(const std::string original_url);
  std::string resetUri(const std::string original_uri, const std::string longest_prefix);
  void log_request_response(std::string request_url, std::string response);
  void log_location(std::string ip_address);
  tcp::socket socket_;
  std::map <std::string, std::pair<std::string, std::shared_ptr<RequestHandler>>> handler_map;
  boost::asio::streambuf buffer;
  std::mutex mutex;
};

#endif
