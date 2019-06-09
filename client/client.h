//
// Created by vitalya on 19.05.19.
//

#ifndef OS_NET_CLIENT_H
#define OS_NET_CLIENT_H

#include "lib/epoll_wrapper.h"
#include "lib/socket_wrapper.h"

#include <atomic>
#include <memory>
#include <string>

class client {
public:
    client(char* addr, int port);
    ~client() = default;

    void start();
    void stop();

private:
    struct sockaddr_in address;
    std::string server_address;
    std::atomic_bool down_flag = {false};

    epoll_wrapper epoll;
    std::unique_ptr<epoll_event[]> events;
    socket_wrapper client_socket;

    static const int MAX_EVENTS;
};

#endif //OS_NET_CLIENT_H
