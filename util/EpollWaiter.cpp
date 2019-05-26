//
// Created by andrey on 26/05/19.
//

#include <iostream>
#include "EpollWaiter.h"

EpollWaiter::EpollWaiter() {
    epfd = epoll_create1(0);
    if (epfd == -1) {
        throw EpollException("Cannot create epoll.", errno);
    }
}

void EpollWaiter::addHandler(std::shared_ptr<IHandler> const &handler) {
    epoll_event event{};
    event.data.fd = handler->getFD();
    if (handler->getActions() & IHandler::WAIT_INPUT) {
        event.events = EPOLLIN;
    }
    if (handler->getActions() & IHandler::WAIT_OUTPUT) {
        event.events = EPOLLOUT;
    }
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, handler->getFD(), &event) == -1) {
        throw EpollException("Cannot add handler.", errno);
    }
    handlers[handler->getFD()] = handler;
}

void EpollWaiter::wait() {
    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];

    while (!handlers.empty()) {
        int nEvents = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nEvents == -1) {
            throw EpollException("epoll_wait failed", errno);
        }
        for (int i = 0; i < nEvents; i++) {
            auto handler = handlers[events[i].data.fd];
            try {
                if (events->events & EPOLLERR) {
                    handler->handleError(*this);
                } else {
                    handler->handleData(*this);
                }
            } catch (HandlerException const &e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}

void EpollWaiter::deleteHandler(int fd) {
    unregisterHandler(fd);
    handlers.erase(fd);
}

void EpollWaiter::deleteAll() {
    for (auto const &handler : handlers) {
        unregisterHandler(handler.first);
    }
    handlers.clear();
}

void EpollWaiter::unregisterHandler(int fd) {
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        throw EpollException("Cannot delete handler.", errno);
    }
}
