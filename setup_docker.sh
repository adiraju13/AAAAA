#kills the running process, and creates a new web-server to run
docker kill "$(docker ps -q)"
docker stop "$(docker ps -a -q)"
docker rm "$(docker ps -a -q)"
docker rmi "$(docker images -a -q)"
docker load -i webserver_img
docker run -d -p 80:2020 httpserver:latest ./web-server simple_config
echo build successfully deployed
exit