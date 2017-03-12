#kills the running process, and creates a new web-server to run
docker kill "$(docker ps -q)"
docker load -i webserver_img
docker run -d -p 80:2020 httpserver ./web-server simple_config
exit