#include <stdlib.h>
#include <iostream>
#include <unistd.h>

#include <signal.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "config_parser/config_parser.h"
#include "utils.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

int main(int argc, char* argv[]) {

 	// only one arg to server possible
    if (argc != 2)
    {
      std::cerr << "Usage: " << argv[0] << " <config-file>\n";
      return 1;
    }

    NginxConfigParser parser;
    NginxConfig config;
    // check config file
    if (!parser.Parse(argv[1], &config)){
      std::cerr << "Error: malformed config file." << std::endl;
      return 1;
    }

    ServerInfo info = utils::setup_info_struct(config);

    // no handlers or no port
    if (!utils::is_valid(info)) {
        return 1;
    }

    pid_t pid = fork();

    // child process, run webserver
    if (pid == 0) {
        std::string cmd = "./web-server ";
        std::string arg(argv[1]);
        cmd += arg;
        system(cmd.c_str());

    // parent process, listen for config changes
    } else if (pid > 0) {
        int length;
        char buffer[BUF_LEN];

        int fd = inotify_init();
        if (fd < 0) {
          std::cerr << "inotify_init error" << std::endl;
          return 1;
        }

        // watch if config is modified
        int wd = inotify_add_watch(fd, argv[1], IN_MODIFY);
        std::cout << "Watching config file name: " << argv[1] << std::endl;

        while(true) {
            // read events in
            length = read( fd, buffer, BUF_LEN );

            if (length < 0) {
              std::cerr << "error when reading events" << std::endl;
              return 1;
            }

            // while events left to read
            for(int i = 0; i < length;) {
                struct inotify_event* event = (struct inotify_event*) &buffer[i];

                // if modify event
                if ( event->mask & IN_MODIFY ) {
                    std::cout << "Config was modified, attempting restart web-server" << std::endl;

                    // check config file
                    if (!parser.Parse(argv[1], &config)){
                      std::cerr << "Error: malformed config file." << std::endl;
                      return 1;
                    }

                    ServerInfo info = utils::setup_info_struct(config);

                    // no handlers or no port
                    if (!utils::is_valid(info)) {
                        return 1;
                    }

                    // kill old web-server
                    system("pkill web-server");
                    sleep(1);

                    // rerun webserver in its own process
                    pid = fork();

                    // child process, run webserver
                    if (pid == 0) {
                        std::string cmd = "./web-server ";
                        std::string arg(argv[1]);
                        cmd += arg;
                        system(cmd.c_str());
                        exit(0);
                    }

                    // else continue listening for file change events...
                }
                // increment
                i += EVENT_SIZE + event->len;
            }
        }
        // clean up
        (void) inotify_rm_watch(fd, wd);
        (void) close(fd);

        return 0;

    } else {
        std::cerr << "Fork Failed!" << std::endl;
        return 1;
    }

    return 0;
}