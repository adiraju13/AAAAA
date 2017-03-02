#include "response_parser.h"
#include <iostream>

std::unique_ptr<Response> ResponseParser::Parse(std::string response) {
	auto output_response = std::unique_ptr<Response>(new Response());

	size_t index = 0;

	// First line of response should have format like "HTTP/1.0 200 OK"
	// Find first space and second space to get status code
	index = response.find(" ");
	std::string rest_of_status_line = response.substr(index + 1);

	// Find the response code
	index = rest_of_status_line.find(" ");
	std::string status_code = rest_of_status_line.substr(0, index);
	if (!is_digits(status_code)) {
		output_response->SetStatus(static_cast<Response::ResponseCode >(std::stoi("")));
	}
	else {
		// Status code should be all digits
		output_response->SetStatus(static_cast<Response::ResponseCode >(std::stoi(status_code)));
	}

	// Get the rest of the line with headers
	index = rest_of_status_line.find("\r\n");
	response = response.substr(index);

	// Find each line of a header and its value separated by a "\r\n"
	while ((index = response.find("\r\n")) != std::string::npos) {
		std::string header_line = response.substr(0, index);

		// If a line is empty, then there are no headers
		if (header_line.length() == 0) 
			break;

		// Look for colon that separates a header
		size_t colon = header_line.find(":");
		if (colon != std::string::npos) {

			// Get header name
			std::string header_name = header_line.substr(0, colon);

			// Get header value up to the first "\r\n"
			size_t end = header_line.find("\r\n");
			std::string header_value = header_line.substr(colon + 2, end);

			// Add headers into response vector
			output_response->AddHeader(header_name, header_value);
		}

		// Cut off the now processed line of headers so that on the next iteration,
		// the next header will be found
		response = response.substr(index + 2);
	}

	return output_response;
}

// Function that determines whether every element of a string is a digit
bool ResponseParser::is_digits(const std::string &str) 
{
    return str.find_first_not_of("0123456789") == std::string::npos;
}