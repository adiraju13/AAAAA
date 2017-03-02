#include "../response_parser.h"
#include "gtest/gtest.h"

TEST(ResponseParserTest, ResponseParseTest) {
	ResponseParser response_parser;
	std::string response_string = ""; 
	std::unique_ptr<Response> response_ptr;

	response_string += std::string("HTTP/1.1 200 OK\r\n") +
	"Content-Type: text/css\r\n" + 
	"Last-Modified: Thu, 19 Jan 2017 19:09:51 GMT\r\n" + 
	"Vary: Accept-Encoding\r\n" + 
	"ETag: W/\"58810eff-c8cc\"\r\n" +
	"Expires: Fri, 02 Mar 2018 02:35:15 GMT\r\n" + 
	"Cache-Control: max-age=31536000, max-age=31536000, public\r\n" +
	"Pragma: public\r\n" +
	"Date: Thu, 02 Mar 2017 02:35:38 GMT\r\n"
	"Age: 23\r\n" +
	"Connection: close\r\n" +
	"X-Cache: HIT\r\n\r\n" + 
	"This is my test body";

	// only parses out the headers and status
	// the body should equal empty string
	response_ptr = response_parser.Parse(response_string); 

	std::string headers[11]; 

	int i = 0; 

    std::vector<std::pair<std::string, std::string>> resp_headers = response_ptr->GetHeaders(); 	
    for(std::vector<std::pair<std::string, std::string>>::const_iterator it = resp_headers.begin(); it != resp_headers.end(); it++) {
    	std::string name = it->first; 
    	std::string value = it->second;
        headers[i] = name + ": " + value;
        i++;
    }

	EXPECT_EQ(response_ptr->GetStatus(), Response::ResponseCode::OK);
	EXPECT_EQ(headers[0], "Content-Type: text/css");
	EXPECT_EQ(headers[1], "Last-Modified: Thu, 19 Jan 2017 19:09:51 GMT");
	EXPECT_EQ(headers[2], "Vary: Accept-Encoding");
	EXPECT_EQ(headers[3], "ETag: W/\"58810eff-c8cc\"");
	EXPECT_EQ(headers[4], "Expires: Fri, 02 Mar 2018 02:35:15 GMT");
	EXPECT_EQ(headers[5], "Cache-Control: max-age=31536000, max-age=31536000, public");
	EXPECT_EQ(headers[6], "Pragma: public");
	EXPECT_EQ(headers[7], "Date: Thu, 02 Mar 2017 02:35:38 GMT");
	EXPECT_EQ(headers[8], "Age: 23");
	EXPECT_EQ(headers[9], "Connection: close");
	EXPECT_EQ(headers[10], "X-Cache: HIT");									
	EXPECT_EQ(response_ptr->GetBody(), "");
	
}