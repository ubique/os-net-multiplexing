//
// Created by domonion on 02.05.19.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <zconf.h>
#include <utils.hpp>
#include <vector>
#include <random>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>

using std::cout;
using std::endl;
using std::mt19937;
using std::vector;

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
    mt19937 randomer(std::chrono::system_clock::now().time_since_epoch().count());
    int s = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    check_error(s, "socket");
    struct sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    check_error(inet_pton(AF_INET, argv[1], &server.sin_addr), "inet_pton");
    check_error(set_nonblock(s), "nonblock");
    if (connect(s, (sockaddr *) (&server), sizeof(server)) == -1 && errno != EINPROGRESS)
        check_error(-1, "connect");
    vector<char> message(MAX_MESSAGE_LEN);
    for (char &i : message)
        i = 'A' + randomer() % 26;
    message.back() = '\0';
    cout << &message[0] << endl;
    int EPoll = epoll_create1(0);
    check_error(EPoll, "epoll_create1");
    struct epoll_event Event;
    Event.data.fd = s;
    Event.events = EPOLLOUT;
    check_error(epoll_ctl(EPoll, EPOLL_CTL_ADD, s, &Event), "epoll_ctl");
    struct epoll_event res;
    int N = epoll_wait(EPoll, &res, 1, -1);
    check_error(N, "epoll_wait");
    if (Event.events & EPOLLOUT) {
        doSend(&message[0], MAX_MESSAGE_LEN, s);
    } else {
        cout << "something went totally wrong!";
        return -1;
    }
    Event.data.fd = s;
    Event.events = EPOLLIN;
    check_error(epoll_ctl(EPoll, EPOLL_CTL_MOD, s, &Event), "epoll_ctl");
    N = epoll_wait(EPoll, &res, 1, -1);
    check_error(N, "epoll_wait");
    if (Event.events & EPOLLIN) {
        doRecv(&message[0], MAX_MESSAGE_LEN, s);
        cout << &message[0] << endl;
        check_error(shutdown(s, SHUT_RDWR), "shutdown");
        check_error(close(s), "close");
    } else {
        cout << "something went much more wrong!";
    }
    return 0;
}
