#ifndef UTILS_DEFINED
#define UTILS_DEFINED

#include "config_parser/config_parser.h"
#include "handler.h"
#include <map>

// server info struct
struct ServerInfo {
    short port = -1;

    // default value
    size_t num_threads = 6;

    // map that maps url prefix to pair of handler name and pointer
    // Ex. "/echo" -> EchoHandler
    std::map <std::string, std::pair<std::string, std::shared_ptr<RequestHandler>>> handler_map;
};

namespace utils {

    // returns a struct containing all config info nicely parsed
    ServerInfo setup_info_struct(NginxConfig config);

    // returns true if server info struct contains correct fields
    bool is_valid(ServerInfo info);

    void writeHandlersToFile(std::map<std::string, std::string>handler_names);
}

#endif
