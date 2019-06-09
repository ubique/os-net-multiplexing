//
// Created by vitalya on 27.05.19.
//

#ifndef OS_NET_EPOLL_WRAPPER_H
#define OS_NET_EPOLL_WRAPPER_H

#include <sys/epoll.h>
#include <unistd.h>

class epoll_wrapper {
public:
    epoll_wrapper();
    ~epoll_wrapper();

    void start();

    void stop();

    void process(int fd, uint32_t event_type, int mode);

    int wait(struct epoll_event* events, int max_events_number);

public:
    static const int MAX_CONNECTIONS;

private:
    int fd;
};


#endif //OS_NET_EPOLL_WRAPPER_H
