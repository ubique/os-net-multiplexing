#ifndef OS_NET_MULTIPLEX_SELECTMULTIPLEXOR_H
#define OS_NET_MULTIPLEX_SELECTMULTIPLEXOR_H

#include <map>

#include "Multiplexor.h"

class SelectMultiplexor : public Multiplexor {
public:
    SelectMultiplexor();
    ~SelectMultiplexor() override;

    virtual void add_fd(int fd, int ops, void* attachment) override ;
    virtual void remove_fd(int fd) override ;
    virtual int select(int timeout_ms) override ;
    virtual std::tuple<int, int, void*> next() override ;

protected:
    std::map<int, void*> attachments;

private:
    struct fdset2 {
        fd_set selected;
        fd_set ready;
    };

    fdset2 fds_read;
    fdset2 fds_write;
    fdset2 fds_other;

    int fd_max_selected = 0;
    int fd_ready_cnt = 0;
    int fd_cur = 0;
};


#endif //OS_NET_MULTIPLEX_SELECTMULTIPLEXOR_H
