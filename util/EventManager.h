#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cstring>
#include <sys/socket.h>


#ifdef __FreeBSD__
typedef errno_t error_t;
#endif


inline error_t getError(int fd) {
    error_t error;
    socklen_t errlen;
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *) &error, &errlen) == 0) {
        return error;
    } else {
        return 0;
    }
}


class EventException : public std::exception {
    std::string msg;
public:
    explicit EventException(std::string const &msg, error_t error)
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


class EventManager;

class IHandler {
public:
    virtual void handleData(EventManager &eventManager) = 0;
    virtual void handleError(EventManager &eventManager) = 0;
    virtual int getFD() = 0;

    virtual ~IHandler() = default;
};


class EventManager {
public:
    EventManager();

    void addHandler(const std::shared_ptr<IHandler> &handler);

    void deleteHandler(int fd);

    void deleteAll();

    void wait();

private:
    void unregisterHandler(int fd);

    int epfd;
    std::map<int, std::shared_ptr<IHandler>> handlers;
};
