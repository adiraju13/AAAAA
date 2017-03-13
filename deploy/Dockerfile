FROM busybox:ubuntu-14.04
WORKDIR /opt/httpserver
COPY . /opt/httpserver
EXPOSE 2020:2020
CMD ["./supervisor", "simple_config"]
