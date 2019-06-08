//
// Created by daniil on 08.06.19.
//

#ifndef OS_NET_MULTIPLEXING_SERVER_H
#define OS_NET_MULTIPLEXING_SERVER_H

#include <stdexcept>
#include <cstring>
#include "Socket.h"
#include "Epoll.h"

struct server_error : std::runtime_error {
    server_error(const std::string &message) : std::runtime_error(message + strerror(errno)) {}
};

class Server {
    const int BUFFER_SIZE = 2048;
    const int EVENTS_SIZE = 1024;
    Socket server_socket;
    Epoll epoll;
public:
    Server(const char* addres, uint16_t port);
    void run();
    void deleteSocket(Socket& socket);
};


#endif //OS_NET_MULTIPLEXING_SERVER_H
