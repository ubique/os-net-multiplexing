//
// Created by Михаил Терентьев on 2019-05-23.
//

#ifndef OS_NET_CLIENT_H
#define OS_NET_CLIENT_H

#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>

using namespace std;

struct wrapper {
    explicit wrapper(int discriptor) : descriptor(discriptor) {}


    ~wrapper() {
        close(descriptor);
    }

    wrapper(wrapper const &) = delete;

    wrapper &operator=(int fd) {
        this->descriptor = fd;
        return *this;
    }

    wrapper &operator=(wrapper const &) = delete;

    bool isBroken() {
        return descriptor == -1;
    }

    int const &getDescriptor() {
        return descriptor;
    }

private:
    int descriptor;
};

class client {
public:
     void send(int desc, std::string const& message);
    client(std::string const &address, std::string const &port_representation);
    void run();
private:
    void insrt_epoll(int fd, uint32_t events);
    void fillAddress(std::string const &address,int port);

private:
    bool isInterrupted = false;
    bool connected;
    wrapper socket_fd,epollfd;
    struct sockaddr_in server;
    static const size_t BF_SZ = 1024;
    static const size_t MAX_EVENTS = 2;
    struct epoll_event main_event, events[MAX_EVENTS];
    char server_reply[BF_SZ];
};


#endif //OS_NET_CLIENT_H
