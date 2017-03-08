#!/bin/bash

# build webserver and get web-server binary
docker build -t httpserver.build .
docker run httpserver.build > binary.tar
tar -xf binary.tar
mv web-server deploy

# build shrunk image and save it
docker build -t httpserver deploy
docker save -o webserver_img httpserver

# assuming that the image file has been compressed and is named webserver_img, and that the image file and key are in the same directory as the make file
chmod 400 AAAAA-webserver.pem
scp -i AAAAA-webserver.pem webserver_img ec2-user@ec2-52-37-251-244.us-west-2.compute.amazonaws.com:~
cat setup_docker.sh | ssh -i AAAAA-webserver.pem ec2-user@ec2-52-37-251-244.us-west-2.compute.amazonaws.com
