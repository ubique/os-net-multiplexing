//
// Created by domonion on 02.05.19.
//
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
#include <utils.hpp>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <set>

using std::set;
using std::cout;
using std::endl;
using std::string;
using std::vector;

const int MAX_EVENTS = 10;

int set_nonblock(int fd) {
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

int main(int argc, char **argv) {
    if (argc != 2) {
        cout << "Usage: ./server address\nExample: ./server 127.0.0.1";
        return 0;
    }
    int master = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
    check_error(master, "socket");
    set_nonblock(master);
    struct sockaddr_in server{}, client{};
    socklen_t size = sizeof(sockaddr_in);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    check_error(inet_pton(AF_INET, argv[1], &server.sin_addr), "inet_pton");
    check_error(bind(master, (sockaddr *) (&server), size), "bind");
    check_error(listen(master, SOMAXCONN), "listen");
    int EPoll = epoll_create1(0);
    check_error(EPoll, "epoll_create1");
    struct epoll_event Event;
    Event.data.fd = master;
    Event.events = EPOLLIN;
    check_error(epoll_ctl(EPoll, EPOLL_CTL_ADD, master, &Event), "epoll_ctl");
    vector<char> data(MAX_MESSAGE_LEN);
    while (true) {
        struct epoll_event Events[MAX_EVENTS];
        int N = epoll_wait(EPoll, Events, MAX_EVENTS, -1);
        check_error(N, "epoll_wait");
        for (int i = 0; i < N; i++) {
            if (Events[i].data.fd == master) {
                int slave = accept(master, (sockaddr *) (&client), &size);
                check_error(set_nonblock(slave), "set_nonblock");
                check_error(slave, "accept");
                struct epoll_event slaveEvent;
                slaveEvent.data.fd = slave;
                slaveEvent.events = EPOLLIN;
                check_error(epoll_ctl(EPoll, EPOLL_CTL_ADD, slave, &slaveEvent), "epoll_ctl");
            } else {
                int slave = Events[i].data.fd;
                if (Events[i].events & EPOLLIN) {
                    doRecv(&data[0], MAX_MESSAGE_LEN, slave);
                    cout << &data[0] << endl;
                    for (char &i : data) {
                        i += abs('A' - 'a');
                    }
                    data.back() = '\0';
                    struct epoll_event slaveEvent;
                    slaveEvent.data.fd = slave;
                    slaveEvent.events = EPOLLOUT;
                    check_error(epoll_ctl(EPoll, EPOLL_CTL_MOD, slave, &slaveEvent), "epoll_ctl");
                }
                if (Events[i].events & EPOLLOUT) {
                    doSend(&data[0], MAX_MESSAGE_LEN, slave);
                    check_error(shutdown(slave, SHUT_RDWR), "shutdown");
                    check_error(close(slave), "close");
                }
            }
        }
    }
}