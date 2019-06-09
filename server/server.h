//
// Created by vitalya on 19.05.19.
//

#ifndef OS_NET_SERVER_H
#define OS_NET_SERVER_H

#include "lib/epoll_wrapper.h"
#include "lib/socket_wrapper.h"

#include <arpa/inet.h>
#include <atomic>
#include <cstdint>
#include <deque>
#include <string>
#include <memory>
#include <unordered_map>


class server {
public:
    server(char* addr, int port);

    server(const server& other) = delete;
    server& operator=(const server& other) = delete;

    void start();
    void stop();

private:
    struct sockaddr_in address;
    std::atomic_bool down_flag = {false};
    int server_fd = -1;
    std::string server_address;

    epoll_wrapper epoll;
    std::unique_ptr<epoll_event[]> events;
    std::unordered_map<int, socket_wrapper> sockets;
    std::unordered_map<int, std::deque<std::string>> responses;

    static const int MAX_EVENTS;
};


#endif //OS_NET_SERVER_H
