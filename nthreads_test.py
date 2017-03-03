import socket
import sys, os
import subprocess
import time
import signal


def mult_threads_test(num_threads):
	"""
	Opens webserver and makes num_threads-1 requests to blocking
	handlers that block forever. Then sends echo request to prove
	that the nth thread is still serviceable.
	"""
	try:
		port = 8080

		# create temp file as config for test
		file_name = "temp_config"
		file = open(file_name, 'w')
		file.write("port {0};\n".format(port))
		file.write("num_threads {0};\n".format(num_threads))
		file.write("path /echo EchoHandler {}\n")
		file.write("path /block BlockingHandler {}\n")
		file.write("default NotFoundHandler {}\n")
		file.close()

		# start server
		server = subprocess.Popen("exec ./web-server " + file_name, shell=True)
		time.sleep(2)

		host = 'localhost'
		buffer_size = 1024

		# open num_threads-1 blocking handlers
		for i in xrange(num_threads - 1):

			request = 'GET /block HTTP/1.1\r\n\r\n'
			soc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			soc.connect((host, port))
			soc.send(request)
			print "Opened Blocking Handler Number " + str(i)


		# initiate echo request after n-1 threads are occupied with blocking handlers
		request = 'GET /echo HTTP/1.1\r\n\r\n'
		soc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		soc.connect((host, port))
		soc.send(request)
		print "Sent Echo Request While Blocking Handlers Open"

		backend_response = soc.recv(buffer_size)
		soc.close()
		print "Received: " + backend_response

		server.kill()

		# True if we received a response while n-1 blockinghandlers were being serviced
		return len(backend_response) > 0

	except socket.timeout:
		server.kill()
		return False

	except KeyboardInterrupt:
		server.kill()

	# clean up temp file
	finally:
		os.remove(file_name)

if __name__ == '__main__':

	# read num_threads from the command line
	num_threads = int(sys.argv[1])
	result = mult_threads_test(num_threads)

	if result:
		print "\nNThreads Test Passed"
	else:
		print "\nNThreads Test Failed"