#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include "Utils.h"

int sock;
int poll;
const int CNT_TRY = 50;

int main(int argc, char** argv, char** env) {
    if (argc > 2) {
        print_error("A lot of arguments");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in addr{};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        print_error("Can't create a socket");
        exit(EXIT_FAILURE);
    }

    set_non_blocking(sock);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int st = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
    poll = epoll_create1(0);
    epoll_event events[CNT_EVENTS];
    if (poll == -1) {
        print_error("Can't create a poll");
        close(sock);
        exit(EXIT_FAILURE);
    }
    bool in_prog = errno == EINPROGRESS;
    if (st == -1 && !in_prog) {
        print_error("Error in connect");
        close(sock);
        close(poll);
        exit(EXIT_FAILURE);
    } else if (st == -1) {
        if (epoll_ctl_wrap(poll, EPOLL_CTL_ADD, sock, EPOLLOUT) == -1) {
            print_error("Can't add socket in poll");
            close(sock);
            close(poll);
            exit(EXIT_FAILURE);
        }
        int cnt = 0;
        while (true) {
            cnt = epoll_wait(poll, reinterpret_cast<epoll_event *>(&events), CNT_EVENTS, -1);
            if (cnt == -1) {
                if (errno == EINTR) {
                    continue;
                }
                print_error("Wait in progress failed");
                close(sock);
                close(poll);
                exit(EXIT_FAILURE);
            }
            int stat;
            socklen_t len = sizeof(int);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &stat, &len) == -1) {
                print_error("Can't getseckopt");
                close(sock);
                close(poll);
                exit(EXIT_FAILURE);
            }
            if (stat != 0) {
                print_error("Can't connect");
                close(sock);
                close(poll);
                exit(EXIT_FAILURE);
            }
            if (stat == 0) {
                break;
            }
        }
        if (delete_socket(poll, sock) == -1) {
            print_error("Can't delete socket from poll");
            close(sock);
            close(poll);
            exit(EXIT_FAILURE);
        }
    }

    if (epoll_ctl_wrap(poll, EPOLL_CTL_ADD, sock, EPOLLIN) == -1) {
        print_error("Can't add a socket in poll");
        close(sock);
        close(poll);
        exit(EXIT_FAILURE);
    }

    set_non_blocking(0);
    if (epoll_ctl_wrap(poll, EPOLL_CTL_ADD, 0, EPOLLIN) ) {
        print_error("Can't add a stdin in poll");
        close(sock);
        close(poll);
        exit(EXIT_FAILURE);
    }

    while(true) {
        int cnt = epoll_wait(poll, reinterpret_cast<epoll_event *>(&events), CNT_EVENTS, -1);
        if (cnt == -1) {
            continue;
        }
        for(int i = 0; i < cnt; i++) {
            if (events[i].data.fd == sock) {
                if (events[i].events & EPOLLIN) {
                    std::string response;
                    read(sock, response);
                    std::cout << response << std::endl;
                }
            } else if (events[i].data.fd == 0) {
                if (events[i].events & EPOLLIN) {
                    std::string str;
                    std::getline(std::cin, str);
                    write(sock, str);
                }
            } else {
                print_error("Unknown fd");
            }
        }
    }
}