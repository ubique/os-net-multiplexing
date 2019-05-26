#pragma once

#include <vector>
#include <map>
#include <sys/epoll.h>
#include <memory>
#include <cstring>
#include <sys/socket.h>


inline error_t getError(int fd) {
    error_t error;
    socklen_t errlen;
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *) &error, &errlen) == 0) {
        return error;
    } else {
        return 0;
    }
}


class EpollException : public std::exception {
    std::string msg;
public:
    explicit EpollException(std::string const &msg, error_t error)
            : msg(msg + "\nError: " + strerror(error)) {}

    char const *what() const noexcept override {
        return msg.c_str();
    }
};


class HandlerException : public std::exception {
    std::string msg;
public:
    explicit HandlerException(std::string const &msg, error_t error)
            : msg(msg + "\nError: " + strerror(error)) {}

    char const *what() const noexcept override {
        return msg.c_str();
    }
};


class EpollWaiter;

class IHandler {
public:
    virtual void handleData(EpollWaiter &waiter) = 0;
    virtual void handleError(EpollWaiter &waiter) = 0;
    virtual int getFD() = 0;
    virtual uint32_t getActions() = 0;

    virtual ~IHandler() = default;

    static const uint32_t WAIT_INPUT = 1;
    static const uint32_t WAIT_OUTPUT = 2;
};


class EpollWaiter {
public:
    EpollWaiter();

    void addHandler(const std::shared_ptr<IHandler> &handler);

    void deleteHandler(int fd);

    void deleteAll();

    void wait();

private:
    void unregisterHandler(int fd);

    int epfd;
    std::map<int, std::shared_ptr<IHandler>> handlers;
};