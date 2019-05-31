#ifndef OS_MULTIPLEXING_FILE_DESCRIPTOR_H
#define OS_MULTIPLEXING_FILE_DESCRIPTOR_H

#include <unistd.h>

class file_descriptor {
public:
    file_descriptor(){};

    explicit file_descriptor(int fd) : fd(fd) {};

    ~file_descriptor() {
        close(fd);
    }

    operator int() {
        return fd;
    }


    int get() {
        return fd;
    }

    file_descriptor &operator=(int d) {
        this->fd = d;
        return *this;
    }
private:
    int fd;
};

#endif //OS_MULTIPLEXING_FILE_DESCRIPTOR_H
