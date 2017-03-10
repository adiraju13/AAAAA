LDFLAGS=-static-libgcc -static-libstdc++ -pthread -Wl,-Bstatic -lboost_system -lboost_thread
CXXFLAGS=-Wall -Werror -std=c++11 -fprofile-arcs -ftest-coverage

# directory locations
CP_LOC=config_parser/
MARKDOWN_LOC=cpp-markdown/
GTEST_DIR=googletest/googletest
GMOCK_DIR=googletest/googlemock

# test locations
SESSION_TEST=test/session_test.cpp session.cpp
UTILS_TEST=test/utils_test.cpp
SERVER_TEST=test/server_test.cpp
PARSER_TEST=test/config_parser_test.cc
HANDLER_TEST=test/handler_test.cpp
REQUEST_TEST=test/http_request_test.cpp
RESPONSE_TEST=test/http_response_test.cpp
RESPONSE_PARSER_TEST=test/response_parser_test.cpp

OBJECT_FILES=http_request.o http_response.o utils.o config_parser.o handler.o response_parser.o

all: server.o session.o main.o config_parser.o markdown.o markdown-tokens.o utils.o http_request.o http_response.o handler.o response_parser.o
	g++ -o web-server main.o server.o session.o config_parser.o markdown.o markdown-tokens.o utils.o http_request.o http_response.o handler.o response_parser.o $(LDFLAGS) $(CXXFLAGS) -lboost_regex -lboost_iostreams

server.o: server.cpp server.h
	g++ -c server.cpp $(LDFLAGS) $(CXXFLAGS)

session.o: session.cpp session.h
	g++ -c session.cpp $(LDFLAGS) $(CXXFLAGS)

config_parser.o: $(CP_LOC)config_parser.cc $(CP_LOC)config_parser.h
	g++ -c $(CP_LOC)config_parser.cc -g $(CXXFLAGS)

markdown.o: $(MARKDOWN_LOC)markdown.cpp $(MARKDOWN_LOC)markdown.h
	g++ -c $(MARKDOWN_LOC)markdown.cpp -g $(CXXFLAGS)

markdown-tokens.o: $(MARKDOWN_LOC)markdown-tokens.cpp $(MARKDOWN_LOC)markdown-tokens.h
	g++ -c $(MARKDOWN_LOC)markdown-tokens.cpp -g $(CXXFLAGS)

utils.o: utils.cpp utils.h
	g++ -c utils.cpp $(LDFLAGS) $(CXXFLAGS)

main.o: main.cpp server.h session.h $(CP_LOC)config_parser.h utils.h
	g++ -c main.cpp $(LDFLAGS) $(CXXFLAGS)

http_request.o: http_request.h http_request.cpp
	g++ -c http_request.cpp $(LDFLAGS) $(CXXFLAGS)

http_response.o: http_response.h http_response.cpp
	g++ -c http_response.cpp $(LDFLAGS) $(CXXFLAGS)

handler.o: handler.h handler.cpp
	g++ -c handler.cpp $(LDFLAGS) $(CXXFLAGS)

response_parser.o: response_parser.h response_parser.cpp
	g++ -c response_parser.cpp $(LDFLAGS) $(CXXFLAGS)

.PHONY: clean, all, test, deploy

deploy:
	./deploy.sh

test:
	# Build web-server for integration test
	make
	# Build gtest
	g++ -std=c++0x -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -pthread -c ${GTEST_DIR}/src/gtest-all.cc
	ar -rv libgtest.a gtest-all.o
	# Build tests
	g++ -isystem ${GTEST_DIR}/include ${SESSION_TEST} ${GTEST_DIR}/src/gtest_main.cc libgtest.a $(OBJECT_FILES) -o session_test ${LDFLAGS} ${CXXFLAGS}
	g++ -isystem ${GTEST_DIR}/include ${UTILS_TEST} ${GTEST_DIR}/src/gtest_main.cc libgtest.a $(OBJECT_FILES) -o utils_test ${LDFLAGS} ${CXXFLAGS}
	g++ -isystem ${GTEST_DIR}/include ${PARSER_TEST} ${GTEST_DIR}/src/gtest_main.cc libgtest.a $(OBJECT_FILES) -o config_parser_test ${LDFLAGS} ${CXXFLAGS}
	g++ -isystem ${GTEST_DIR}/include ${SERVER_TEST} ${GTEST_DIR}/src/gtest_main.cc libgtest.a $(OBJECT_FILES) -o server_test ${LDFLAGS} ${CXXFLAGS}
	g++ -isystem ${GTEST_DIR}/include ${HANDLER_TEST} ${GTEST_DIR}/src/gtest_main.cc libgtest.a $(OBJECT_FILES) -o handler_test ${LDFLAGS} ${CXXFLAGS}
	g++ -isystem ${GTEST_DIR}/include ${REQUEST_TEST}   ${GTEST_DIR}/src/gtest_main.cc libgtest.a ${OBJECT_FILES} -o  http_request_test ${LDFLAGS} ${CXXFLAGS}
	g++ -isystem ${GTEST_DIR}/include ${RESPONSE_TEST}    ${GTEST_DIR}/src/gtest_main.cc libgtest.a ${OBJECT_FILES} -o  http_response_test ${LDFLAGS} ${CXXFLAGS}
	g++ -isystem ${GTEST_DIR}/include ${RESPONSE_PARSER_TEST} ${GTEST_DIR}/src/gtest_main.cc libgtest.a ${OBJECT_FILES} -o response_parser_test ${LDFLAGS} ${CXXFLAGS}

	# Run Tests
	./handler_test
	./config_parser_test
	./server_test
	./session_test
	./utils_test
	./http_request_test
	./http_response_test
	./response_parser_test
	python2 integration_multithread_test.py
	python2 proxy_test.py
	python2 nthreads_test.py 5
clean:
	# Note: be careful of make clean removing *_test
	rm -f *.o *.gcno *.gcov *.gcda web-server *_test
