#include "handler.h"
#include "http_response.h"
#include "http_request.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>


// Dynamic handler creation code
std::map<std::string, RequestHandler* (*)(void)>* request_handler_builders = nullptr;

RequestHandler* RequestHandler::CreateByName(const char* type) {
  const auto type_and_builder = request_handler_builders->find(type);
  if (type_and_builder == request_handler_builders->end()) {
    return nullptr;
  }
  return (*type_and_builder->second)();
}

//////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// Blocking Handler  /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

RequestHandler::Status BlockingHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
    return RequestHandler::PASS;
}

RequestHandler::Status BlockingHandler::HandleRequest(const Request& request, Response* response){
    std::cout << "\nBlockingHandler::HandleRequest" << std::endl;

    while (true) {}
    return RequestHandler::PASS;
}

//////////////////////////////////////////////////////////////////////////////////////
////////////////////////////  Not Found Handler  /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

RequestHandler::Status NotFoundHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
    return RequestHandler::PASS;
}

RequestHandler::Status NotFoundHandler::HandleRequest(const Request& request, Response* response){
    std::cout << "\nNotFoundHandler::HandleRequest" << std::endl;

    std::string body = "404 NOT FOUND";
    response->SetStatus(Response::NOT_FOUND);
    response->AddHeader("Content-Length", std::to_string(body.size()));
    response->AddHeader("Content-Type", "text/plain");
    response->SetBody(body);
    return RequestHandler::PASS;
}

//////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Echo Handler /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

RequestHandler::Status EchoHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
    return RequestHandler::PASS;
}

RequestHandler::Status EchoHandler::HandleRequest(const Request& request, Response* response) {
    std::cout << "\nEchoHandler::HandleRequest" << std::endl;

    response->SetStatus(Response::OK);
    response->AddHeader("Content-Length", std::to_string(request.raw_request().size() - 4));
    response->AddHeader("Content-Type", "text/plain");
    response->SetBody(request.raw_request());
    return RequestHandler::PASS;
}

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Static Handler /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

// Sets dir_from_config to directory specified after root in config
RequestHandler::Status StaticHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
    for (unsigned int i = 0; i < config.statements_.size(); i++){
        // get the root from the child block
        if (config.statements_[i]->tokens_[0] == "root" && config.statements_[i]->tokens_.size() == 2){
            serve_path = config.statements_[i]->tokens_[1];
        }
    }

    this->uri_prefix = uri_prefix;

    //Code to add the / between the file path and serve_path properly
    if (serve_path.length() != 0 && serve_path[serve_path.length()-1] != '/'){
        if (uri_prefix.length() != 0 && uri_prefix[uri_prefix.length()-1] == '/'){
            serve_path += '/';
        }
    }else{
        if (uri_prefix.length() != 0 && uri_prefix[uri_prefix.length()-1] != '/'){
            if (serve_path.length() != 0){
                serve_path = serve_path.substr(0, serve_path.length() - 1);
            }
        }
    }

    return RequestHandler::PASS;
}

// Attempts to serve file that was requested by the request object
// If not possible, we return fail signal for outside class to handle
// calling not found handler
RequestHandler::Status StaticHandler::HandleRequest(const Request& request, Response* response) {
    std::cout << "\nStaticHandler::HandleRequest" << std::endl;

    std::string abs_path = serve_path;

    //get the file name/path from the request url
    std::string file_path = request.uri().substr(uri_prefix.length());
    abs_path += file_path;

    if(!is_regular_file(abs_path.c_str())) {
      std::cerr << "Error: " << abs_path << " does not exist" << std::endl;
      return RequestHandler::FAIL;
    }

    // save content_type header based on requested file extension
    std::string content_type = get_content_type(abs_path);
    // raw byte array
    std::string to_send = read_file(abs_path);

    // if markdown file extension
    if (abs_path.size() > 3 && abs_path.substr(abs_path.size() - 3, 3) == ".md") {
      // read in .md file
      markdown::Document d;
      d.read(to_send);

      // write out as .html
      std::ostringstream output;
      d.write(output);

      to_send = output.str();
    }

    std::cout << "Serving file from: " << abs_path << std::endl;

    response->SetStatus(Response::OK);
    response->AddHeader("Content-Length", std::to_string(to_send.size()));
    response->AddHeader("Content-Type", content_type);
    response->SetBody(to_send);
    return RequestHandler::PASS;
}

// checks if file exists and is regular file
bool StaticHandler::is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}


// gets content-type based on the file extension of requested file
std::string StaticHandler::get_content_type(std::string filename) {
    unsigned int i;

    // search for either last period or last slash in filename
    for (i = filename.size() - 1; i >= 0; i--) {
        // file with no extension type
        if (filename[i] == '/') {
            return "text/plain";
        } else if (filename[i] == '.') { // found pos of beginning of extension
            break;
        }
    }

    std::string ext = filename.substr(i + 1, std::string::npos);
    std::string content_type;

    // based on ext decide content_type header
    if (ext == "html" || ext == "md") {
        content_type = "text/html";
    } else if (ext == "jpg") {
        content_type = "image/jpeg";
    } else if (ext == "pdf") {
        content_type = "application/pdf";
    } else {
        content_type = "text/plain";
    }
    return content_type;
}

// reads raw file into vector of characters
std::string StaticHandler::read_file(std::string filename) {
    std::ifstream ifs(filename, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    std::vector<char> result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(&result[0], pos);

    std::string str_rep(result.begin(), result.end());

    return str_rep;
}


//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Status Handler /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


RequestHandler::Status StatusHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
    return RequestHandler::PASS;
}

//reads the log file of handled requests and their corresponding responses and fills a map with them
bool StatusHandler::addHandledRequests() {
    std::ifstream infile("request_response_log.txt");

    if (!infile.is_open())
        return false;

    std::string request_url;
    int response_code;
    while (infile >> request_url >> response_code)
    {
        map_of_request_and_responses[request_url].push_back(response_code);
    }

    infile.close();

    return true;
}

//reads the log file of handler names and their corresponding uri's and fills a map with them
bool StatusHandler::addHandlerMapping() {
    std::ifstream infile("handler_names.txt");

    if (!infile.is_open())
        return false;

    std::string name, url;
    while (infile >> name >> url)
    {
        map_of_uri_to_handler[name] = url;
    }

    infile.close();

    return true;
}

//Adds the handler names mapping and handler response to reqeust mapping that were created above
//to the message response body and creates the response object.
RequestHandler::Status StatusHandler::HandleRequest(const Request& request, Response* response) {
    if (!addHandledRequests() || !addHandlerMapping()){
        return RequestHandler::FAIL;
    }

    std::cout << "\nStatusHandler::HandleRequest" << std::endl;

    std::string to_send;

    to_send += "These are the handlers available and their URIs:\n";
    for (auto const& x : map_of_uri_to_handler){
        to_send += x.first;
        to_send += " -> ";
        to_send += x.second;
        to_send += "\n";
    }

    to_send += "\nThese are the requests received and their corresponding response codes\n";
    for (auto const& x : map_of_request_and_responses){
        for(auto const& c: x.second){
            to_send += x.first;
            to_send += " -> ";
            to_send += std::to_string(c);
            to_send += "\n";
        }

    }
    response->SetStatus(Response::OK);
    response->AddHeader("Content-Length", std::to_string(to_send.length()));
    response->AddHeader("Content-Type", "text/plain");
    response->SetBody(to_send);

    map_of_uri_to_handler.clear();
    map_of_request_and_responses.clear();

    return RequestHandler::PASS;
}

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Proxy Handler /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

RequestHandler::Status ProxyHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {

    for (unsigned int i = 0; i < config.statements_.size(); i++){
        // get the root from the child block
        if (config.statements_[i]->tokens_[0] == "host" && config.statements_[i]->tokens_.size() == 2){
            m_host = config.statements_[i]->tokens_[1];
        }
        else if (config.statements_[i]->tokens_[0] == "port" && config.statements_[i]->tokens_.size() == 2){
            std::cout << "port: " << m_port << std::endl;
            m_port = config.statements_[i]->tokens_[1];
        }
    }

    this->uri_prefix = uri_prefix;

    return RequestHandler::PASS;
}

void ProxyHandler::SetHost(std::string host) {
    m_host = host;
}

std::unique_ptr<Response> ProxyHandler::get_response(std::string path, std::string host_address, std::string port_num)
{

  using boost::asio::ip::tcp;
  //if we hit the cache
  /*
  if (cache.count(path)){
    std::cout << "CACHE-HIT" << std::endl;
    std::pair <time_t, std::string> cache_value;
    time_t time_now;
    time(&time_now);
    //value of cache is still valid
    if (time_now < cache_value.first){
        std::unique_ptr<Response> response_ptr;
        int pos_end_of_headers = cache_value.second.find("/r/n/r/n");
        std::string headers = cache_value.second.substr(0, pos_end_of_headers);
        response_ptr = response_parser.Parse(headers);
        response_ptr->SetBody(cache_value.second.substr(pos_end_of_headers + 4));
        return response_ptr;
    }
  }
  */
  try
  {
    boost::asio::io_service io_service;
    boost::system::error_code error;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(host_address, port_num);
    std::cout << host_address << std::endl;
    std::cout << port_num << std::endl;
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_service);
    error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
    if (error) {
      std::cout << "Error connecting to host: " << host_address << std::endl;
      return nullptr;
    }

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    std::string request_str = "GET " + path + " HTTP/1.0\r\n\r\n";

    request_stream << request_str;

    // Send the request.
    boost::asio::write(socket, request);

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    std::unique_ptr<Response> response_ptr;

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n", error);
    if (error) {
        std::cout << "Error reading until end of response\n";
        return nullptr;
    }

    std::string s( (std::istreambuf_iterator<char>(&response)), std::istreambuf_iterator<char>() );
    response_ptr = response_parser.Parse(s);

    // Check if there is a "Location" header, signaling redirect
    bool cache_enabled = false;
    int time_to_cache = 0;
    std::string redirect_path = "";
    for (auto &header: response_ptr->GetHeaders()) {
        if (header.first == "Location") // Get the redirect path
            redirect_path = header.second;
        if (header.first == "Cache-Control"){
            std::pair<bool, int> cache_control_info = check_cache_control_field(header.second);
            cache_enabled = cache_control_info.first;
            time_to_cache = cache_control_info.second;
            std::cout << "time to cache = " << time_to_cache << std::endl;
        }
    }

    // Handle redirect
    if ((response_ptr->GetStatus() == 302) && !redirect_path.empty()) {
        // Find whether rediret path is http or https
        std::string http_path = redirect_path.substr(1, redirect_path.find(":") - 1);
        // Assume redirect location has format "http://www.something.com/"
        redirect_path = redirect_path.substr(redirect_path.find("//") + 2);

        // Find "www.something.com" btween the slashes
        redirect_path = redirect_path.substr(0, redirect_path.find("/"));

        // Redirect can be "https" this requires port 443 instead of 80
        std::string host_ = redirect_path;

        // Set port of https to 443 so it doesn't loop
        if (http_path == "https")
            return get_response(path, host_, "443");
        else    // regular http requests are on port 80
            return get_response(path, host_, "80");
    }

    // Read until EOF, writing data to output as we go.
    while (boost::asio::read(socket, response,
          boost::asio::transfer_at_least(1), error)){
            if (error)
                break;
        }

    // Read the rest of the response which will be the response body
    // Add it with the headers we got before to form a complete response
    std::string body( (std::istreambuf_iterator<char>(&response)), std::istreambuf_iterator<char>() );
    std::string whole_response = s + body;

    // Set the response body to only the body, without the headers
    response_ptr->SetBody(whole_response.substr(whole_response.find("\r\n\r\n") + 4));
    if (cache_enabled){
        add_to_cache(path, time_to_cache, whole_response);
    }
    return response_ptr;
  }
  catch (std::exception& e)
  {
    std::cout << "Exception: " << e.what() << "\n";
  }
  return nullptr;
}

RequestHandler::Status ProxyHandler::HandleRequest(const Request& request, Response* response) {
    std::cout << "\nProxyHandler::HandleRequest" << std::endl;

    // Pass in the request uri
    std::string request_uri = request.uri();
    std::unique_ptr<Response> returned_ptr = get_response(request_uri, m_host, m_port);

    if (returned_ptr == nullptr) {
        return RequestHandler::FAIL;
    }
    else {
        response->SetStatus(returned_ptr->GetStatus());
        for (auto &header: returned_ptr->GetHeaders()) {
            response->AddHeader(header.first, header.second);
        }
        response->SetBody(returned_ptr->GetBody());
    }
    return RequestHandler::PASS;
}

void ProxyHandler::add_to_cache(std::string path, int time_to_cache, std::string whole_response){
    time_t expiration_time;
    time(&expiration_time);
    expiration_time += time_to_cache;
    cache.push_back(std::make_pair(path, whole_response));
    std::cout << "made it to here" << std::endl;

}

std::pair <bool, int> ProxyHandler::check_cache_control_field(std::string field){

    if (field.find("public") == std::string::npos || field.find("no_cache") != std::string::npos){
        std::pair<bool, int> error_pair;
        error_pair.first = false;
        error_pair.second = 0;
        return error_pair;
    }
   
    //check if max_age is given
    int time_to_cache = 0;
    if (field.find("max-age") == std::string::npos){
        time_to_cache = 30000000;
    }
    else{
        
        int pos = field.find("max-age=");
        std::string t_t_c = field.substr(pos+8);
        t_t_c = t_t_c.substr(0, t_t_c.find(','));
        time_to_cache = stoi(t_t_c);
    }

    std::pair<bool, int> to_return;
    to_return.first = true;
    to_return.second = time_to_cache;
    return to_return;
} 
