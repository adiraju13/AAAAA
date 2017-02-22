#include "handler.h"
#include "http_response.h"
#include "http_request.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>


//////////////////////////////////////////////////////////////////////////////////////
////////////////////////////  Not Found Handler  /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

RequestHandler::Status NotFoundHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
    return RequestHandler::PASS;
}

RequestHandler::Status NotFoundHandler::HandleRequest(const Request& request, Response* response){
    response->SetStatus(Response::NOT_FOUND);
    response->AddHeader("Content-Length", std::to_string(request.raw_request().size() - 4));
    response->AddHeader("Content-Type", "text/plain");
    response->SetBody("404 NOT FOUND");
    return RequestHandler::PASS;
}

//////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Echo Handler /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

RequestHandler::Status EchoHandler::Init(const std::string& uri_prefix, const NginxConfig& config) {
    return RequestHandler::PASS;
}

RequestHandler::Status EchoHandler::HandleRequest(const Request& request, Response* response) {
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

    return RequestHandler::PASS;
}

// Attempts to serve file that was requested by the request object
// If not possible, we return fail signal for outside class to handle
// calling not found handler
RequestHandler::Status StaticHandler::HandleRequest(const Request& request, Response* response) {
    std::string abs_path = get_exec_path() + serve_path + "/" + request.uri();
    std::cout << "Attempting to serve file from: " << abs_path << std::endl;

    if (!file_exists(abs_path)) {
      std::cerr << "Error: " << abs_path << " does not exist" << std::endl;
      return RequestHandler::FAIL;
    }

    // save content_type header based on requested file extension
    std::string content_type = get_content_type(abs_path);

    // raw byte array
    std::string to_send = read_file(abs_path);

    response->SetStatus(Response::OK);
    response->AddHeader("Content-Length", std::to_string(to_send.size()));
    response->AddHeader("Content-Type", content_type);
    response->SetBody(to_send);
    return RequestHandler::PASS;
}

// gets current path of executable
std::string StaticHandler::get_exec_path() {
    // max path is 2048 characters in file directory
    const int MAX_PATH = 2048;
    char buffer[MAX_PATH];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        return std::string(buffer);
    }
    else {
        std::cerr << "Error: unable to find current working directory" << std::endl;
    }
    return "";
}

// checks if file exists
bool StaticHandler::file_exists(std::string filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
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
    if (ext == "html") {
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
std::string StaticHandler::read_file(std::string filename)
{
    std::ifstream ifs(filename, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    std::vector<char> result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(&result[0], pos);

    std::string str_rep(result.begin(), result.end());

    return str_rep;
}
