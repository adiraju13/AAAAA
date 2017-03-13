#include "utils.h"
#include "handler.h"
#include <fstream>
#include <iostream>

namespace utils {


ServerInfo setup_info_struct(NginxConfig config) {

    ServerInfo info;

    //map of uri handlers to names
    std::map<std::string, std::string> handlers_for_status;
    std::string status_uri;

    // parse away our config file into info struct
    for (unsigned int i = 0; i < config.statements_.size(); i++) {

        std::string curr_statement = config.statements_[i]->tokens_[0];

        if (curr_statement == "port" && config.statements_[i]->tokens_.size() == 2) {
            info.port = std::atoi(config.statements_[i]->tokens_[1].c_str());
        } else if (curr_statement == "num_threads" && config.statements_[i]->tokens_.size() == 2) {
            info.num_threads = std::atoi(config.statements_[i]->tokens_[1].c_str());
        }
        else if (curr_statement == "path" && config.statements_[i]->tokens_.size() == 3) {

            std::string uri_prefix = config.statements_[i]->tokens_[1];
            std::string handler_name = config.statements_[i]->tokens_[2];

            if (handler_name == "StatusHandler"){
                status_uri = uri_prefix;
            }

            handlers_for_status[handler_name] = uri_prefix;

            auto raw_handler_ptr = RequestHandler::CreateByName(handler_name.c_str());
            std::shared_ptr<RequestHandler> handler_ptr (raw_handler_ptr);

            handler_ptr->Init(uri_prefix, *(config.statements_[i]->child_block_));

            info.handler_map[uri_prefix] = std::make_pair(handler_name, std::move(handler_ptr));

        } else if (curr_statement == "default" && config.statements_[i]->tokens_.size() == 2) {
            std::string default_uri = "";
            std::string handler_name = config.statements_[i]->tokens_[1];

            auto raw_handler_ptr = RequestHandler::CreateByName(handler_name.c_str());
            std::shared_ptr<RequestHandler> handler_ptr (raw_handler_ptr);

            handler_ptr->Init(default_uri, *(config.statements_[i]->child_block_));
            info.handler_map[default_uri] = std::make_pair(handler_name, std::move(handler_ptr));
        }
    }
    //such that empty string is mapped to NotFoundHandler
    info.handler_map[""] = std::make_pair("NotFoundHandler", std::move(std::shared_ptr<RequestHandler>(new NotFoundHandler)));

    if (status_uri != ""){
        writeHandlersToFile(handlers_for_status);
    }

    return info;
}


//write the handler name to uri mapping to a file so that status handler can read it later
void writeHandlersToFile(std::map<std::string, std::string>handler_names){
    std::ofstream f("handler_names.txt");
    for (auto const &pair : handler_names){
        std::string to_write = "";
        to_write = pair.first + " " + pair.second + "\n";
        f.write(to_write.c_str(), to_write.length());
    }
    f.close();
}

// returns true if info object is valid server config
bool is_valid(ServerInfo info) {
    if (info.port < 0) {
        std::cerr << "Port number was not specified in config\n";
        return false;
    } else if (info.handler_map.empty()) {
        std::cerr << "No handlers were specified for server\n";
        return false;
    }

    return true;
}





} // end utils namespace
