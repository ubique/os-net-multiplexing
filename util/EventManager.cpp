//
// Created by andrey on 26/05/19.
//

#include <iostream>
#include "EventManager.h"

#ifdef __FreeBSD__
#include <sys/event.h>
#else
#include <sys/epoll.h>
#endif


EventManager::EventManager() {
#ifdef __FreeBSD__
    epfd = kqueue();
    if (epfd == -1) {
        throw EpollException("Cannot create kqueue.", errno);
    }
#else
    epfd = epoll_create1(0);
    if (epfd == -1) {
        throw EpollException("Cannot create epoll.", errno);
    }
#endif
}

void EventManager::addHandler(std::shared_ptr<IHandler> const &handler) {
    int fd = handler->getFD();
#ifdef __FreeBSD__
    struct kevent event{};
    EV_SET(&event, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    if (kevent(epfd, &event, 1, NULL, 0, NULL) == -1) {
        throw EpollException("Cannot add handler.", errno);
    }
#else
    struct epoll_event event{};
    event.data.fd = fd;
    event.events |= EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1) {
        throw EpollException("Cannot add handler.", errno);
    }
#endif

    handlers[handler->getFD()] = handler;
}

void EventManager::wait() {
    const int MAX_EVENTS = 10;
#ifdef __FreeBSD__
    struct kevent events[MAX_EVENTS];
#else
    struct epoll_event events[MAX_EVENTS];
#endif
    while (!handlers.empty()) {
#ifdef __FreeBSD__
        int nEvents = kevent(epfd, NULL, 0, events, MAX_EVENTS, NULL);
#else
        int nEvents = epoll_wait(epfd, events, MAX_EVENTS, -1);
#endif
        if (nEvents == -1) {
            throw EpollException("waiting failed", errno);
        }
        for (int i = 0; i < nEvents; i++) {
            try {
#ifdef __FreeBSD__
                auto handler = handlers[events[i].ident];
                bool isError = events[i].flags & EV_ERROR;
#else
                auto handler = handlers[events[i].data.fd];
                bool isError = events[i].events & EPOLLERR;
#endif
                if (isError) {
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

void EventManager::deleteHandler(int fd) {
    unregisterHandler(fd);
    handlers.erase(fd);
}

void EventManager::deleteAll() {
    for (auto const &handler : handlers) {
        unregisterHandler(handler.first);
    }
    handlers.clear();
}

void EventManager::unregisterHandler(int fd) {
#ifdef __FreeBSD__
    struct kevent event{};
    EV_SET(&event, fd, 0, EV_DELETE, 0, 0, NULL);
    if (kevent(epfd, &event, 1, NULL, 0, NULL) == -1) {
        throw EpollException("Cannot delete handler.", errno);
    }
#else
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        throw EpollException("Cannot delete handler.", errno);
    }
#endif
}
