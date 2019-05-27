#include "utils.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <numeric>
#include <fcntl.h>
#include <sys/epoll.h>

void print_error(std::string const& message) {
    std::cerr << message << std::strerror(errno) << std::endl;
}

bool check_port(const char* port) {
    uint32_t value = 0;
    for (const char *ptr = port; *ptr != 0; ptr++) {
        if (!isdigit(*ptr)) {
            return false;
        }
        value = value * 10 + (*ptr - '0');
        if (value > std::numeric_limits<uint16_t>::max()) {
            return false;
        }
    }
    return true;
}

bool set_nonblocking(int descriptor) {
    int flags = fcntl(descriptor, F_GETFL, 0);
    if (flags == -1) {
        print_error("fcntl failed: ");
        return false;
    }
    if (fcntl(descriptor, F_SETFL, flags | O_NONBLOCK) == -1) {
        print_error("fcntl failed: ");
        return false;
    }
    return true;
}

bool add_epoll(int epoll_descriptor, int descriptor, epoll_event & event) {
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, descriptor, &event) == -1) {
        print_error("epoll_ctl failed: ");
        return false;
    }
    return true;
}
