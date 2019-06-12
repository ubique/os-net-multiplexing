#pragma once

#include <bits/exception.h>
#include <stdexcept>
#include <sys/epoll.h>


class Server {
public:
    Server(char *address, uint16_t port);

    ~Server();

    void start();

private:
    int mySocket;
    int myEpoll;
    epoll_event *events = new epoll_event[6];
    static constexpr size_t BUFFER_SIZE = 1024;

    void openSocket();

    void setUp(char *address, uint16_t port);

    void createEpoll();

    bool doSend(char message[], int sock, ssize_t size);

};


