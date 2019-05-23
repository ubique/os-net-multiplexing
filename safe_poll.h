#ifndef OS_NET_MULTIPLEXING_SAFE_POLL_H
#define OS_NET_MULTIPLEXING_SAFE_POLL_H


#include <poll.h>
#include <cstdio>

void try_poll(struct pollfd *fds, nfds_t nfds, int timeout);

int try_read(int fd, void *buf, size_t nbytes);

#endif //OS_NET_MULTIPLEXING_SAFE_POLL_H
