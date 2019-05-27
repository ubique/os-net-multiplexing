#ifndef OS_NET_MULTIPLEX_EPOLLMULTIPLEXOR_H
#define OS_NET_MULTIPLEX_EPOLLMULTIPLEXOR_H

#if defined(USE_EPOLL)

#include "Multiplexor.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include <map>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>


class EpollMultiplexor : public Multiplexor {
public:
    static constexpr int MAX_EVENTS = 1024;

    EpollMultiplexor();
    ~EpollMultiplexor() override;

    virtual void add_fd(int fd, int ops, void* attachment) override ;
    virtual void remove_fd(int fd) override ;
    virtual int select(int timeout_ms) override ;
    virtual std::tuple<int, int, void*> next() override ;

private:
    struct my_data{
        int fd;
        void* attachment;
    };

    std::map<int, my_data*> datas;

    int epollfd;
    std::vector<std::unique_ptr<epoll_event>> epollset;

    struct epoll_event events[MAX_EVENTS];

    int fds_left = 0;
    int fd_cur = 0;
};


#endif // USE_EPOLL

#endif //OS_NET_MULTIPLEX_EPOLLMULTIPLEXOR_H
