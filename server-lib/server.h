//
// Created by Михаил Терентьев on 2019-05-23.
//

#ifndef OS_NET_SERVER_H
#define OS_NET_SERVER_H


#include <iostream>
#include <stdexcept>
#include <vector>

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/epoll.h>


struct wrapper {
    explicit wrapper(int fd) : descriptor(fd) {}

    wrapper(wrapper const &) = delete;

    wrapper &operator=(wrapper const &) = delete;

    bool isBroken() {
        return descriptor == -1;
    }

    int const &getDescriptor() {
        return descriptor;
    }

    ~wrapper() {
        close(descriptor);
    }

private:
    int descriptor;
};

struct server {

    server(std::string const &address, std::string const &port_representation);

    server &operator=(server const &) = delete;

    [[noreturn]] void run();

private:
    void fillAddress(std::string const &address,int port);
    
private:
    wrapper epoll_fd, socket_fd;
    struct sockaddr_in server_in{};
    static const size_t MAX_EVENTS = 1024;
    struct epoll_event main_event, events[MAX_EVENTS];
    static const size_t BF_SZ = 1024;
};

#endif //OS_NET_SERVER_H
