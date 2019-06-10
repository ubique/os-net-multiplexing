#include <iostream>
#include <fcntl.h>
#include <netinet/in.h>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <set>
#include "Utils.h"

int listener;
int poll;


void stop() {
    if (shutdown(listener, SHUT_RDWR) == -1) {
        print_error("Can't shutdown a server");
    }
    if (close(listener) == -1) {
        print_error("Can't close a master socket");
    }
    if (close(poll) == -1) {
        print_error("Can't close a poll socket");
    }
}


int main(int argc, char** argv, char** env) {
    if (argc > 2) {
        print_error("A lot of arguments");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr{};

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0) {
        print_error("Can't create socket");
        exit(EXIT_FAILURE);
    }

    set_non_blocking(listener);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        print_error("Can't bind a server");
        close(listener);
        exit(EXIT_FAILURE);
    }
    listen(listener, CNT_USERS);

    poll = epoll_create1(0);
    if (epoll_ctl_wrap(poll, EPOLL_CTL_ADD, listener, EPOLLIN | EPOLLERR) == -1) {
        print_error("Can't add a listen socket in poll");
        stop();
        exit(EXIT_FAILURE);
    }
    epoll_event events[CNT_EVENTS];
    while(true) {
        log("waiting...");
        int cnt = epoll_wait(poll, reinterpret_cast<epoll_event*>(&events), CNT_EVENTS, -1);
        if (cnt == -1) {
            log("Count = -1");
            continue;
        }
        for(int i = 0; i < cnt; i++) {
            if (events[i].data.fd == listener) {
                if (events[i].events & EPOLLIN) {
                    int sock = accept(listener, nullptr, nullptr);
                    log("Accepted");
                    if(sock < 0) {
                        print_error("Can't accept a socket");
                        continue;
                    }
                    set_non_blocking(sock);
                    if (epoll_ctl_wrap(poll, EPOLL_CTL_ADD, sock, EPOLLIN | EPOLLHUP) == -1) {
                        print_error("Can't add a client socket in poll");
                        close(sock);
                        continue;
                    }
                }
            } else if (events[i].data.fd != 0) { // client socket
                std::string request;
                if (read(events[i].data.fd, request) == -1) {
                    print_error("Can't receive data from socket!");
                    continue;
                }
                std::string response = request.append(" (from server)");
                if(write(events[i].data.fd, request) == -1) {
                    print_error("Can't send a response to client");
                    continue;
                }
            }
        }
    }
}

