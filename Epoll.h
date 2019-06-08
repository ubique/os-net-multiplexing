//
// Created by daniil on 26.05.19.
//

#ifndef OS_NET_MULTIPLEXING_EPOLL_H
#define OS_NET_MULTIPLEXING_EPOLL_H

#include <sys/epoll.h>

struct epoll_exception : std::runtime_error {
    epoll_exception(std::string const& cause)
            : std::runtime_error(cause + ": " + strerror(errno)) {}
};


class Epoll {
    int data;
    const int EVENT_SIZE = 10;
public:
    Epoll();

    void start();

    bool check();

    void close();

    int doEvent(int fd, uint32_t flags, int mode);

    int wait(struct epoll_event* events);

    ~Epoll();
};


#endif //OS_NET_MULTIPLEXING_EPOLL_H
