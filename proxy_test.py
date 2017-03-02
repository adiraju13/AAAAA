import socket
import sys, os
import subprocess
import time
import signal


def proxy_test(port1, port2):
	try:
		server1 = subprocess.Popen("exec ./web-server test_config", shell=True)
		server2 = subprocess.Popen("exec ./web-server proxy_config", shell=True)

		time.sleep(2)

		host = 'localhost'
		buffer_size = 1024

		request = 'GET /index.html HTTP/1.1\r\n\r\n'
		# proxy = 'GET / HTTP/1.1\r\n\r\n'

		socket1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)	#backend
		socket2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)	#proxy

		socket1.connect((host, port1))
		socket1.send(request)
		print "Sent Request to Backend"

		backend_response = socket1.recv(buffer_size)
		socket1.close()
		print "Completed Request To Backend"


		socket2.connect((host, port2))
		socket2.send(request)
		print "Sent Request Through Proxy To Backend"

		proxy_response = socket2.recv(buffer_size)
		socket2.close()
		print "Completed Request Through Proxy to Backend"

		server1.kill()
		server2.kill()

		print backend_response
		print proxy_response
		print "backend response length: " + str(len(backend_response))
		print "proxy to backend response length: " + str(len(proxy_response))

		return proxy_response==backend_response

	except socket.timeout:
		server.kill()
		return False

	except KeyboardInterrupt:
		server.kill()

port1 = 2020;
port2 = 2030;

file = open("test_config", 'w')
file.write("port {0};\n".format(port1))
file.write("path / StaticHandler {\n    root public;\n}\n")
file.write("path /proxy ProxyHandler {\n    host ucla.edu;\n    port 2030;\n}\n")
file.write("path /echo EchoHandler {}\n")
file.write("path /status StatusHandler {}\n")
file.write("default NotFoundHandler {}\n")
file.close()

file = open("proxy_config", 'w')
file.write("port {0};\n".format(port2))
file.write("path /static StaticHandler {\n   root public;\n}\n")
file.write("path / ProxyHandler {\n    host localhost;\n    port 2020;\n}\n")
file.write("path /echo EchoHandler {}\n")
file.write("path /status StatusHandler {}\n")
file.write("default NotFoundHandler {}\n")
file.close()



result = proxy_test(port1, port2)
if result:
	print "\nProxy Test Passed"
else:
	print "\nProxy Test Failed"