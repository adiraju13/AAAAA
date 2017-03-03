from os import system
from threading import Thread
import socket
import time
import sys
import subprocess

def setup_socket():
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	return s;

def client_thread_paused():
	TCP_IP = '127.0.0.1'
	TCP_PORT = 2020
	BUFFER_SIZE = 1024
	MESSAGE = "GET /block HTTP/1.1"

	s = setup_socket()
	s.connect(('localhost', TCP_PORT))
	s.send(MESSAGE)
	s.send("\r\n\r\n")
	data = s.recv(BUFFER_SIZE)
	s.close()

	return

def client_thread(error_code, REQUEST, E_RESPONSE):
	TCP_IP = '127.0.0.1'
	TCP_PORT = 2020
	BUFFER_SIZE = 1024
	MESSAGE = REQUEST

	s = setup_socket()
	s.connect(('localhost', TCP_PORT))
	s.send(MESSAGE)
	data = s.recv(BUFFER_SIZE)
	s.close()

	if (data != E_RESPONSE):
	    	print "Failure response: ", repr(data)
                error_code[0] = 1
		return

	error_code[0] = 0

if __name__ == "__main__":
	print "\n-----Opening server-----"
	p = subprocess.Popen("./web-server ./test/test_config", shell=True)

	time.sleep(4)



	#test functionality of the echo handler
	error_code = [0]
	REQUEST = "GET /echo HTTP/1.1\r\n\r\n"
	EXPECTED_RESPONSE = "HTTP/1.1 200 OK\r\nContent-Length: 18\r\nContent-Type: text/plain\r\n\r\nGET /echo HTTP/1.1\r\n\r\n"
	thread_client_four = Thread(target = client_thread, args=(error_code, REQUEST, EXPECTED_RESPONSE))
	thread_client_four.start()
	thread_client_four.join()

	#tests the functionality of the status handler
	error_code_three = [0]
	REQUEST = "GET /status HTTP/1.1\r\n\r\n"
	EXPECTED_RESPONSE = "HTTP/1.1 200 OK\r\nContent-Length: 231\r\nContent-Type: text/plain\r\n\r\nThese are the handlers available and their URIs:\nBlockingHandler -> /block\nEchoHandler -> /echo\nStaticHandler -> /static\nStatusHandler -> /status\n\nThese are the requests received and their corresponding response codes\n/echo -> 200\n"
	thread_client_three = Thread(target = client_thread, args=(error_code_three, REQUEST, EXPECTED_RESPONSE))
	thread_client_three.start()
	thread_client_three.join()

	#test shows that multithreading works because the first thread is opened then paused
	#and the second thread that was opened finishes before the first finishes.
	#also tests the functionalities of the static handler
	
	thread_client = Thread(target = client_thread_paused, args=())
	thread_client.start()
	
	error_code_two = [0]
	REQUEST = "GET /static/index.html HTTP/1.1\r\n\r\n"
	EXPECTED_RESPONSE = "HTTP/1.1 200 OK\r\nContent-Length: 28\r\nContent-Type: text/html\r\n\r\n<head> Hello World! </head>\n"
	thread_client_two = Thread(target = client_thread, args=(error_code_two, REQUEST, EXPECTED_RESPONSE))
	thread_client_two.start()

	thread_client_two.join()

	print "\n\n-----Closing the server/Doing cleanup-----"
	system("fuser -k 2020/tcp")

	time.sleep(2)

	if (error_code[0] == 1 or error_code_two[0] == 1 or error_code_three[0] == 1):
		print "\n\nTEST FAILURE\n"
		exit(1)
	else:
		print "\n\nTEST SUCCESS\n"
		exit(0)

