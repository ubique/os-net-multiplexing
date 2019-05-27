#pragma once

#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>

void print_error(std::string const& message);
bool check_port(const char* port);
bool set_nonblocking(int descriptor);
bool add_epoll(int epoll_descriptor, int descriptor, epoll_event & event);

class descriptor_wrapper {
    int descriptor;
public:
    descriptor_wrapper(int fd) : descriptor(fd) {}
    ~descriptor_wrapper() {
        if (descriptor != -1 && close(descriptor) == -1) {
            print_error("Can't close descriptor: ");
        }
        descriptor = -1;
    }
    operator int() const {
        return descriptor;
    }
    descriptor_wrapper& operator=(int fd) {
        descriptor = fd;
        return *this;
    }
};
