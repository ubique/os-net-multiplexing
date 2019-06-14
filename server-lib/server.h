//
// Created by Михаил Терентьев on 2019-05-23.
//

#ifndef OS_NET_SERVER_H
#define OS_NET_SERVER_H


#include <iostream>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>


struct wrapper {
    wrapper() {
        descriptor = socket(AF_INET, SOCK_STREAM, 0);
    }

    wrapper(int descriptor) : descriptor(descriptor) {}

    ~wrapper() {
        if (descriptor != -1) {
            close(descriptor);
        }
    }

    wrapper(wrapper&& other) noexcept : descriptor(*other) {
        other.descriptor = -1;
    }

    wrapper& operator=(wrapper&& other) noexcept {
        descriptor = other.descriptor;
        other.descriptor = -1;

        return *this;
    }

    int getDescriptor(){
        return  descriptor;
    }
    int operator*() const {
        return descriptor;
    }

    bool isBroken() const {
        return descriptor == -1;
    }

    wrapper accept() {
        sockaddr_in client_addr = {0};

        socklen_t client_addr_len = sizeof(client_addr);
        return ::accept(descriptor, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
    }

private:
    int descriptor = -1;
};

struct server {
    server(std::string const &address, std::string const &port_representation);

    server &operator=(server const &) = delete;

    [[noreturn]] void run();

private:
    std::string read(int desc);
    void send(int desc, std::string const& message);
    void fillAddress(std::string const &address,int port);
    void remove_from_epoll(int sock_fd);
    void add_to_epoll(int sock_fd, uint32_t events);
private:
    std::unordered_map<int, wrapper> sockets;
    wrapper epoll_fd{}, socket_fd;
    struct sockaddr_in server_in{};
    static const size_t MAX_EVENTS = 1024;
    static const int BACKLOG_QUEUE_SIZE = 50;
    struct epoll_event main_event, events[MAX_EVENTS];
    const size_t BF_SZ = 10 * 4096;
    const size_t TRIES_NUMBER = 10;
};

#endif //OS_NET_SERVER_H
