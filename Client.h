#pragma once

#include <bits/exception.h>
#include <stdexcept>
#include <sys/epoll.h>


class Client {
public:
    Client(char *address, uint16_t port);

    ~Client();

    void run();

private:
    int mySocket;
    int myEpoll;
    epoll_event* events = new epoll_event[6];
    static constexpr size_t BUFFER_SIZE = 1024;

    void openSocket();

    void makeConnection(char *address, uint16_t port);

    void createEpoll();
};


