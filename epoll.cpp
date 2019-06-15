//
// Created by anastasia on 09.06.19.
//

//
// Created by daniil on 26.05.19.
//

#ifndef OS_NET_MULTIPLEXING_EPOLL_H
#define OS_NET_MULTIPLEXING_EPOLL_H

#include <sys/epoll.h>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <iostream>

struct epoll_exception : std::runtime_error {
    explicit epoll_exception(const std::string &cause)
            : std::runtime_error(cause + ": " + strerror(errno)){}

};


class Epoll {
    int data;
    const int EVENT_SIZE = 10;
public:
    Epoll() : data(-1) {};

    void start() {
        data = epoll_create(1);
        if (data == -1) {
            throw epoll_exception("Can't create epoll");
        }
    }

    bool check() {
        return data != -1;
    }

    void close() {
        if (::close(data) == -1) {
            throw epoll_exception("Can't close epoll");
        }
    }

    int check_ctl(int socket, uint32_t events, int mode) {
        struct epoll_event event{};
        event.events = events;
        event.data.fd = socket;
        int ans;
        if (mode == EPOLL_CTL_MOD || mode == EPOLL_CTL_ADD) {
            ans = ::epoll_ctl(data, mode, socket, &event);
        } else if (mode == EPOLL_CTL_DEL) {
            ans = ::epoll_ctl(data, mode, socket, nullptr);
        } else {
            throw epoll_exception("Unknown operation with epoll");
        }
        if (ans == -1) {
            throw epoll_exception("Can't do epoll ctl");
        }
        return ans;
    }

    int wait(struct epoll_event* events) {
        int size = epoll_wait(data, events, EVENT_SIZE, -1);
        if (size == -1) {
            throw epoll_exception("Epoll wait failed");
        }
        return size;
    }

    ~Epoll() {
        try {
            ::close(data);
        } catch (std::exception &e) {
            std::cerr << "Can't close socket" << std::endl;
        }
    }
};


#endif //OS_NET_MULTIPLEXING_EPOLL_H
