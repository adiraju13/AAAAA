#ifndef HANDLER_DEFINED
#define HANDLER_DEFINED

#include <boost/asio.hpp>
#include "http_response.h"
#include "http_request.h"
#include "response_parser.h"
#include "config_parser/config_parser.h"
#include "cpp-markdown/markdown.h"
#include <vector>

// Represents the parent of all request handlers. Implementation
// is long lived and created at server constrution.
class RequestHandler {
 public:

  // Add to this as needed
  enum Status {
    PASS,
    FAIL
  };

  // to create handlers by the name of the subclass type
  static RequestHandler* CreateByName(const char* type);

  // Initializes the handler. Returns a response code indicating success or
  // failure condition.
  // uri_prefix is the value in the config file that this handler will run for.
  // config is the contents of the child block for this handler ONLY.
  virtual Status Init(const std::string& uri_prefix,
                      const NginxConfig& config) = 0;

  // Handles an HTTP request, and generates a response. Returns a response code
  // indicating success or failure condition. If ResponseCode is not OK, the
  // contents of the response object are undefined, and the server will return
  // HTTP code 500.
  virtual Status HandleRequest(const Request& request,
                               Response* response) = 0;

  Status NotFoundHandler(const Request& request, Response* response);
};

// This is the dynamic object creation helper code that allows us to create handlers based on
// the name of the class
extern std::map<std::string, RequestHandler* (*)(void)>* request_handler_builders;
template<typename T>
class RequestHandlerRegisterer {
 public:
  RequestHandlerRegisterer(const std::string& type) {
    if (request_handler_builders == nullptr) {
      request_handler_builders = new std::map<std::string, RequestHandler* (*)(void)>;
    }
    (*request_handler_builders)[type] = RequestHandlerRegisterer::Create;
  }
  static RequestHandler* Create() {
    return new T;
  }
};
#define REGISTER_REQUEST_HANDLER(ClassName) \
  static RequestHandlerRegisterer<ClassName> ClassName##__registerer(#ClassName)

// simple handler to echo back raw response
class EchoHandler : public RequestHandler {
public:
    EchoHandler() {}

    Status Init(const std::string& uri_prefix,
                const NginxConfig& config);

    Status HandleRequest(const Request& request,
                         Response* response);
private:
    std::string to_send;
};

REGISTER_REQUEST_HANDLER(EchoHandler);

// handler to serve file as response
class StaticHandler : public RequestHandler {
public:
    StaticHandler() {}

    Status Init(const std::string& uri_prefix,
                const NginxConfig& config);

    Status HandleRequest(const Request& request,
                         Response* response);

private:
    //uri_prefix specified by the config file exposed to users
    std::string uri_prefix;
    // path from web-server directory specified in config block
    std::string serve_path;
    // gets path from url string
    std::string get_path_from_url(std::string url);
    // true if file exists
    bool is_regular_file(const char *path);
    // returns content type header based on file extension
    std::string get_content_type(std::string filename);
    // reads raw file into vector of characters
    std::string read_file(std::string filename);
};

REGISTER_REQUEST_HANDLER(StaticHandler);

// handler to display 404 error
class NotFoundHandler : public RequestHandler {
public:
    NotFoundHandler() {}

    Status Init(const std::string& uri_prefix, const NginxConfig& config);

    Status HandleRequest(const Request& request,
                         Response* response);
};


REGISTER_REQUEST_HANDLER(NotFoundHandler);

// handler that blocks forever to help test multithreading code
class BlockingHandler : public RequestHandler {
public:
    BlockingHandler() {}

    Status Init(const std::string& uri_prefix, const NginxConfig& config);

    Status HandleRequest(const Request& request,
                         Response* response);
};


REGISTER_REQUEST_HANDLER(BlockingHandler);

// handler that displays status page for server
class StatusHandler : public RequestHandler {
public:
    StatusHandler() {}

    Status Init(const std::string& uri_prefix, const NginxConfig& config);

    Status HandleRequest(const Request& request,
                         Response* response);

private:
    std::map<std::string, std::vector<int>> map_of_request_and_responses;
    std::map<std::string, std::string> map_of_uri_to_handler;
    bool addHandledRequests();
    bool addHandlerMapping();

};

REGISTER_REQUEST_HANDLER(StatusHandler);

class ProxyHandler : public RequestHandler {
public:
    ProxyHandler() {}

    Status Init(const std::string& uri_prefix,
                const NginxConfig& config);

    Status HandleRequest(const Request& request,
                         Response* response);

    std::unique_ptr<Response> get_response(std::string path, std::string host, std::string port);

    void SetHost(std::string host); // for ease of testing

private:
    //uri_prefix specified by the config file exposed to users
    std::string uri_prefix;
    
    std::string m_host;

    std::string m_port = "80";

    ResponseParser response_parser;

};

REGISTER_REQUEST_HANDLER(ProxyHandler);


#endif
