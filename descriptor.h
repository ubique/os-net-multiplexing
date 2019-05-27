#include <string>
#include <unistd.h>

struct descriptor {
    int fd;

    descriptor(int fd) : fd(fd) {
        if (fd < 0) {
	    perror("Cannot create descriptor");
    	    exit(EXIT_FAILURE);
        }
    }

    ~descriptor() {
        if (close(fd) < 0)
            perror("Cannot close descriptor");
    }

    operator int() {
        return fd;
    }
};
