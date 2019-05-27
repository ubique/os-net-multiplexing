#if defined(USE_EPOLL)

#include <string.h>

#include "EpollMultiplexor.h"

void EpollMultiplexor::add_fd(int fd, int ops, void* attachment) {
    epollset.push_back(std::make_unique<epoll_event>());
    epoll_event& evt = *epollset.back().get();

    evt.events = 0;

    if (ops & OP_READ) evt.events |= EPOLLIN;
    if (ops & OP_WRITE) evt.events |= EPOLLOUT;
    if (ops & OP_EXCEPT) evt.events |= EPOLLERR | EPOLLRDHUP;

    evt.data.ptr = new my_data{fd, attachment};
    datas[fd] = (my_data*) evt.data.ptr;

    int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &evt);

    if (ret < 0) {
        delete(datas[fd]);
        datas.erase(fd);
        throw MultiplexorException(strerror(errno));
    }
}

void EpollMultiplexor::remove_fd(int fd) {
    int ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);

    if (ret < 0) {
        throw MultiplexorException(strerror(errno));
    }

    delete(datas[fd]);
    datas.erase(fd);
}

int EpollMultiplexor::select(int timeout_ms) {
    fds_left = epoll_wait(epollfd, events, MAX_EVENTS, timeout_ms);

    if (fds_left < 0) {
        throw MultiplexorException(strerror(errno));
    } else {
        fd_cur = 0;
    }

    return fds_left;
}

std::tuple<int, int, void*> EpollMultiplexor::next() {
    if (fd_cur >= fds_left) {
        return std::make_tuple(-1, 0, nullptr);
    }

    int mask = events[fd_cur].events;

    my_data* data = static_cast<my_data*> (events[fd_cur].data.ptr);

    int mask_our = 0;
    if (mask & EPOLLIN) mask_our |= OP_READ;
    if (mask & EPOLLOUT) mask_our |= OP_WRITE;
    if (mask & ~EPOLLIN & ~EPOLLOUT) mask_our |= OP_EXCEPT;

    fd_cur++;

    return std::make_tuple(data->fd, mask_our, data->attachment);
}

EpollMultiplexor::EpollMultiplexor() {
    epollfd = epoll_create(16);

    if (epollfd < 0) {
        throw MultiplexorException(std::string("Cannot create epoll fd: ") + strerror(errno));
    }
}

EpollMultiplexor::~EpollMultiplexor() {
    close(epollfd);
}

#endif // USE_EPOLL
