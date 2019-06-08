//
// Created by daniil on 26.05.19.
//

#include <iostream>
#include <unistd.h>
#include <cstring>
#include "Epoll.h"

Epoll::Epoll() : data(-1) {}

void Epoll::start() {
    data = epoll_create(1);
    if (data == -1) {
        throw epoll_exception("Can't create epoll");
    }
}

void Epoll::close() {
    int r = ::close(data);
    if (r == -1) {
        throw epoll_exception("Can't close epoll");
    }
}

Epoll::~Epoll() {
    try {
        ::close(data);
    } catch (std::exception &e) {

    }
}

int Epoll::doEvent(int fd, uint32_t flags, int mode) {
    struct epoll_event event{};
    event.events = flags;
    event.data.fd = fd;
    int ans;
    if (mode == EPOLL_CTL_MOD || mode == EPOLL_CTL_ADD) {
        ans = ::epoll_ctl(data, mode, fd, &event);
    } else if (mode == EPOLL_CTL_DEL) {
        ans = ::epoll_ctl(data, mode, fd, nullptr);
    } else {
        throw epoll_exception("Unknown operation with epoll");
    }
    if (ans == -1) {
        throw epoll_exception("Can't do epoll event");
    }
    return ans;
}

int Epoll::wait(struct epoll_event *events) {
    int size = epoll_wait(data, events, EVENT_SIZE, -1);
    if (size == -1) {
        throw epoll_exception("Epoll wait failed");
    }
    return size;
}

bool Epoll::check() {
    return data != -1;
}


