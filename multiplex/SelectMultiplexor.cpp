#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

#include <utility>
#include <unistd.h>
#include <string.h>

#include "SelectMultiplexor.h"

void SelectMultiplexor::add_fd(int fd, int ops, void* attachment) {
    if (ops & OP_READ) {
        FD_SET(fd, &fds_read.selected);
    }

    if(ops & OP_WRITE) {
        FD_SET(fd, &fds_write.selected);
    }

    if(ops & OP_EXCEPT) {
        FD_SET(fd, &fds_other.selected);
    }

    fd_max_selected = std::max(fd_max_selected, fd);

    attachments[fd] = attachment;
}

void SelectMultiplexor::remove_fd(int fd) {
    FD_CLR(fd, &fds_read.selected);
    FD_CLR(fd, &fds_write.selected);
    FD_CLR(fd, &fds_other.selected);

    attachments.erase(fd);
}

int SelectMultiplexor::select(int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    fds_other.ready = fds_other.selected;
    fds_read.ready = fds_read.selected;
    fds_write.ready = fds_write.selected;

    fd_cur = 0;

    int ret = ::select(fd_max_selected + 1, &fds_read.ready, &fds_write.ready, &fds_other.ready, timeout_ms >= 0 ? &tv : nullptr);

    if (ret < 0) {
        throw MultiplexorException(strerror(errno));
    } else {
        fd_ready_cnt = ret;
    }

    return ret;
}

std::tuple<int, int, void*> SelectMultiplexor::next() {
    while ( fd_ready_cnt > 0 && fd_cur <= fd_max_selected &&
            !(FD_ISSET(fd_cur, &fds_write.ready) ||
            FD_ISSET(fd_cur, &fds_read.ready) ||
            FD_ISSET(fd_cur, &fds_other.ready))
            ) {
        fd_cur++;

    }

    int mask = 0;

    if (FD_ISSET(fd_cur, &fds_read.ready)) {
        mask |= OP_READ;
    }

    if (FD_ISSET(fd_cur, &fds_write.ready)) {
        mask |= OP_WRITE;
    }

    if (FD_ISSET(fd_cur, &fds_other.ready)) {
        mask |= OP_EXCEPT;
    }

    fd_ready_cnt--;

    return std::make_tuple(
            mask == 0 ? -1 : fd_cur++,
            mask,
            mask == 0 ? nullptr : attachments[fd_cur]
    );
}

SelectMultiplexor::SelectMultiplexor() {
    FD_ZERO(&fds_other.selected);
    FD_ZERO(&fds_read.selected);
    FD_ZERO(&fds_write.selected);
}

SelectMultiplexor::~SelectMultiplexor() {

}


