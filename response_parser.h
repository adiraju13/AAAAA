#ifndef RESPONSE_PARSER_H
#define RESPONSE_PARSER_H

#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include "http_response.h"

class ResponseParser {
public:

	ResponseParser() {}

	std::unique_ptr<Response> Parse(std::string response);

private:
	bool is_digits(const std::string &str);
};

#endif