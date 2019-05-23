#include "safe_poll.h"

#include <cstdlib>
#include <cerrno>
#include <error.h>
#include <unistd.h>
#include <cstdio>

void try_poll(struct pollfd *fds, nfds_t nfds, int timeout) {
	int status = poll(fds, nfds, timeout);
	if (status < 0) {
		error(EXIT_FAILURE, errno, "Polling failed");
		return;
	}
	if (status == 0) {
//		error(EXIT_FAILURE, errno, "Polling finished with timeout %d", timeout);
		return;
	}
}

int try_read(int fd, void *buf, size_t nbytes) {
	ssize_t len = read(fd, buf, nbytes);
	if (len == -1) {
		//reading failed
		error(EXIT_FAILURE, errno, "Error during read from %d\n", fd);
		return -1;
	}
	if (len == 0) {
		//socket closed on the other side
		close(fd);
		fprintf(stderr, "End of stream %d\n", fd);
		return 0;
	}
	return len;
}
