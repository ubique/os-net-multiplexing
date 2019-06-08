//
// Created by daniil on 08.06.19.
//

#ifndef OS_NET_MULTIPLEXING_CLIENT_H
#define OS_NET_MULTIPLEXING_CLIENT_H


#include "Socket.h"
#include "Epoll.h"

struct client_error : std::runtime_error {
    client_error(const std::string& message) : std::runtime_error(message) {};
};

class Client {
    Socket client_socket;
    Epoll epoll;
    const int EPOLL_SIZE = 1024;
public:
    Client(char* address, uint16_t port);
    void run();
};


#endif //OS_NET_MULTIPLEXING_CLIENT_H
