//
// Created by vitalya on 27.05.19.
//

#include "epoll_wrapper.h"

#include <stdexcept>

const int epoll_wrapper::MAX_CONNECTIONS = 100;


epoll_wrapper::epoll_wrapper() : fd(-1) {}

epoll_wrapper::~epoll_wrapper() {
    close(fd);
}

void epoll_wrapper::start() {
    fd = epoll_create(MAX_CONNECTIONS);
    if (fd == -1) {
        throw std::runtime_error("Epoll start failed");
    }
}

void epoll_wrapper::stop() {
    int ret = close(fd);
    if (ret == -1) {
        throw std::runtime_error("Epoll close failed");
    }
}

void epoll_wrapper::process(int fd_, uint32_t event_type, int mode) {
    struct epoll_event event{};
    event.events = event_type;
    event.data.fd = fd_;
    int ret = epoll_ctl(fd, mode, fd_, (mode == EPOLL_CTL_DEL ? nullptr : &event));
    if (ret == -1) {
        throw std::runtime_error("Epoll process with \"" + std::to_string(mode) + "\" mode failed");
    }
}

int epoll_wrapper::wait(struct epoll_event* events, int max_events_number) {
    int ret = epoll_wait(fd, events, max_events_number, -1);
    if (ret == -1) {
        throw std::runtime_error("Epoll wait failed");
    }
    return ret;
}


