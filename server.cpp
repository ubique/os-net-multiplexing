//
// Created by Yaroslav on 04/06/2019.
//

#include<bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <errno.h>
#include <zconf.h>
#include <cstring>
#include <string>
#include <vector>
#include <sys/epoll>
#include "utils.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

const int MAX_POLL_SIZE = 16;

void check_error(int rc, const string &problem_name) {
    if (rc == -1) {
        cout << "error occurred with ";
        cout << problem_name << " in server\n";
        cout << strerror(errno) << "\n";
        exit(0);
    }
}

void wrong_usage() {
    cout << "Usage: ./Task5_client [address]";
}

int main(int argc, char **argv) {
    if (argc != 2) {
        wrong_usage();
        return 0;
    }
    int master = socket(AF_INET, SOCK_STREAM, 0);
    check_error(master, "socket");
    struct sockaddr_in server{}, client{};
    socklen_t size_s = sizeof(sockaddr_in);

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    check_error(inet_pton(AF_INET, argv[1], &server.sin_addr), "inet_pton");
    check_error(bind(master, (sockaddr *) (&server), size_s), "bind");
    check_error(listen(master, SOMAXCONN), "listen")

    int poll_s = epoll_create1(0);
    check_error(poll_s, "epoll_create1");
    struct epoll_event Event;

    Event.data.fd = master;
    Event.events = EPOLLIN;
    check_error(epoll_ctl(poll_s, EPOLL_CTL_ADD, master, &Event), "epoll_ctl");
    for(;;) {
        struct epoll_event Events[MAX_POLL_SIZE];
        int limit = epoll_wait(poll_s, Events, MAX_POLL_SIZE, -1);
        check_error(limit, "epoll_wait");
        for (int i = 0; i < limit; i++) {
            if (Events[i].data.fd == master) {
                int s = accept(master, (sockaddr *) (&client), &size_s);
                check_error(s, "accept");
                struct epoll_event epoll_event;
                epoll_event.data.fd = s;
                epoll_event.events = EPOLLIN;
                check_error(epoll_ctl(poll_s, EPOLL_CTL_ADD, s, &epoll_event), "epoll_ctl");
            } else {
                int s = Events[i].data.fd;
                char size;
                char code;
                fun_recv(&size, 1, s, "server");
                code = size % 2 == 1 ? 1 : 0;
                size = size / 2;
                cout << static_cast<int>(size) << endl;
                vector<char> data(size);
                vector<char> copy(size);
                fun_recv(&data[0], size, s, "server");
                cout << &data[0] << endl;
                for (int i = 0; i < data.size(); i++) {
                    copy[i] = data[i + 1];
                }
                copy[size - 1] = '\0';

                if (code == 1) {
                    for (int i = 0; i < size - 1; i++) {
                        copy[i]+=1;
                    }
                } else {
                    for (int i = 0; i < size - 1; i++) {
                        copy[i]-=1;
                    }
                }
                fun_send(&copy[0], size, s, "server");
                check_error(close(s), "close");
            }
        }

    }
}