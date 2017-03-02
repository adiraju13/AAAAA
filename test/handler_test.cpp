#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include "gtest/gtest.h"
#include "../handler.h"


TEST(EchoHandlerTest, EchoClassTest) {
  std::unique_ptr<EchoHandler> handler(new EchoHandler);
  std::string str_req = "GET /echo HTTP/1.1\r\ntest\r\n\r\n";

  auto req = Request::Parse(str_req);

  Response res;
  RequestHandler::Status s = handler->HandleRequest(*req, &res);

  std::string expected = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(str_req.size() - 4) +"\r\nContent-Type: text/plain\r\n\r\n" + str_req;
  EXPECT_EQ(res.ToString(), expected);
  EXPECT_EQ(s, RequestHandler::PASS);
}

TEST(NotFoundHandlerTest, NotFoundClassTest) {
  std::unique_ptr<NotFoundHandler> handler(new NotFoundHandler);
  Request req;
  Response res;
  RequestHandler::Status s = handler->HandleRequest(req, &res);

  std::string body = "404 NOT FOUND";
  std::string expected = "HTTP/1.1 404 Not found\r\nContent-Length: " + std::to_string(body.size()) +"\r\nContent-Type: text/plain\r\n\r\n" + body;
  EXPECT_EQ(res.ToString(), expected);
  EXPECT_EQ(s, RequestHandler::PASS);
}

TEST(StatusHandlerTest, StatusClassTest) {
  std::unique_ptr<StatusHandler> handler(new StatusHandler);

  Request req;
  Response res;

  std::ofstream f("request_response_log.txt");
  std::ofstream t("handler_names.txt");
  f.close();
  t.close();

  RequestHandler::Status s = handler->HandleRequest(req, &res);


  std::string body = "These are the handlers available and their URIs:\n\nThese are the requests received and their corresponding response codes\n";
  std::string expected = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(body.size()) +"\r\nContent-Type: text/plain\r\n\r\n" + body;
  EXPECT_EQ(res.ToString(), expected);
  EXPECT_EQ(s, RequestHandler::PASS);

}


TEST(StaticHandlerTest, StaticClassTest){
  std::unique_ptr<StaticHandler> handler(new StaticHandler);

  std::string str_req = "GET ./public/index.html HTTP/1.1\r\n\r\n";
  auto req = Request::Parse(str_req);
  Response res;

  RequestHandler::Status s = handler->HandleRequest(*req, &res);

  EXPECT_EQ(res.ToString(), "HTTP/1.1 200 OK\r\nContent-Length: 28\r\nContent-Type: text/html\r\n\r\n<head> Hello World! </head>\n");
  EXPECT_EQ(s, RequestHandler::PASS);

}

TEST(ProxyHandlerTest, ProxyClassTest) {
  std::unique_ptr<ProxyHandler> handler(new ProxyHandler); 

  handler->SetHost("ucla.edu"); 

  std::string str_req = "GET / HTTP/1.1\r\n\r\n";
  auto req = Request::Parse(str_req); 
  Response res; 

  RequestHandler::Status s = handler->HandleRequest(*req, &res);
  EXPECT_EQ(s, RequestHandler::PASS);

  auto output = handler->get_response("/", "ucla.edu", "80");
  EXPECT_EQ(output->ToString(), res.ToString());
}
