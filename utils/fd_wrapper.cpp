#include "fd_wrapper.hpp"

#include <sys/epoll.h>

fd_wrapper::fd_wrapper() : descriptor(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) {}

fd_wrapper::fd_wrapper(int descriptor) : descriptor(descriptor) {}

fd_wrapper::~fd_wrapper() {
    close();
}

void fd_wrapper::renew() {
    close();
    descriptor = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
}

int fd_wrapper::get_fd() const {
    return descriptor;
}

bool fd_wrapper::check_valid() const {
    return descriptor != -1;
}

int fd_wrapper::send_message(char const *buf, size_t buffer_size, bool log_success) {
    if (!check_valid()) {
        logger().fail("Socket descriptor is invalid, can not sent message");
        return -1;
    }

    unsigned int EPOLL_MAX_EVENTS = 10;
    epoll_event epoll{}, events[EPOLL_MAX_EVENTS];
    fd_wrapper epoll_desc(epoll_create1(0));
    if (!epoll_desc.check_valid()) {
        logger().fail("Failed to create epoll fd", errno);
        return -1;
    }

    epoll.events = EPOLLOUT;
    epoll.data.fd = descriptor;
    if (epoll_ctl(epoll_desc.get_fd(), EPOLL_CTL_ADD, descriptor, &epoll) < 0) {
        logger().fail("Unable to add socket descriptor to interest list", errno);
        return -1;
    }

    size_t offset = 0;
    while (true) {
        int count = epoll_wait(epoll_desc.get_fd(), events, EPOLL_MAX_EVENTS, -1);
        if (count < 0) {
            logger().fail("Failed to wait for epoll", errno);
            return -1;
        }

        for (unsigned int i = 0; i < count; i++) {
            if (events[i].data.fd == descriptor && (events[i].events & EPOLLOUT)) {
                int status = send_next(buf, buffer_size, offset);
                if (status < 0) {
                    return -1;
                } else if (status == 0) {
                    if (log_success) {
                        logger().success("Sent '" + std::string(buf, offset - 1) +
                                         "' of size " + std::to_string(buffer_size));
                    }
                    return 0;
                }
            }
        }
    }
}

int fd_wrapper::receive_message(char *buf, size_t &buffer_size, unsigned int real_size, bool log_success) {
    if (!check_valid()) {
        logger().fail("Socket descriptor is invalid, can not receive message");
        return -1;
    }

    unsigned int EPOLL_MAX_EVENTS = 10;
    epoll_event epoll{}, events[EPOLL_MAX_EVENTS];
    fd_wrapper epoll_desc(epoll_create1(0));
    if (!epoll_desc.check_valid()) {
        logger().fail("Failed to create epoll fd", errno);
        return -1;
    }

    epoll.events = EPOLLIN;
    epoll.data.fd = descriptor;
    if (epoll_ctl(epoll_desc.get_fd(), EPOLL_CTL_ADD, descriptor, &epoll) < 0) {
        logger().fail("Unable to add socket descriptor to interest list", errno);
        return -1;
    }

    size_t offset = 0;
    while (true) {
        int count = epoll_wait(epoll_desc.get_fd(), events, EPOLL_MAX_EVENTS, -1);
        if (count < 0) {
            logger().fail("Failed to wait for epoll", errno);
            return -1;
        }

        for (unsigned int i = 0; i < count; i++) {
            if (events[i].data.fd == descriptor && (events[i].events & EPOLLIN)) {
                int status = receive_next(buf, buffer_size, real_size, offset);
                if (status < 0) {
                    return -1;
                } else if (status == 0) {
                    if (log_success) {
                        logger().success("Received '" + std::string(buf) + "' of size " + std::to_string(offset));
                    }
                    return 0;
                }
            }
        }
    }
}

void fd_wrapper::close() {
    if (check_valid() && ::close(descriptor) < 0) {
        logger().fail("Could not close the socket properly", errno);
    }
    descriptor = -1;
}

int fd_wrapper::send_next(char const *buf, size_t buffer_size, size_t &offset) {
    ssize_t sent = send(descriptor, reinterpret_cast<void const *>(buf + offset), buffer_size - offset, 0);
    if (sent == -1) {
        logger().fail("Failed to send a message via connection", errno);
        return -1;
    }
    offset += sent;
    if (offset == buffer_size) {
        return 0;
    }
    return 1;
}

int fd_wrapper::receive_next(char *buf, size_t &buffer_size, unsigned int real_size, size_t &offset) {
    ssize_t received = recv(descriptor, reinterpret_cast<void *>(buf + offset), real_size - offset, 0);
    if (received == -1) {
        logger().fail("Failed to receive a message via connection", errno);
        return -1;
    }
    offset += received;
    if (buf[offset - 1] == '\n') {
        buf[offset - 1] = '\0';
        buffer_size = offset - 1;
        return 0;
    }
    return 1;
}

int fd_wrapper::simple_send_message(char const *buf, size_t buffer_size, bool log_success) {
    if (!check_valid()) {
        logger().fail("Socket descriptor is invalid, can not sent message");
        return -1;
    }
    size_t offset = 0;
    while (true) {
        int status = send_next(buf, buffer_size, offset);
        if (status < 0) {
            return -1;
        } else if (status == 0) {
            if (log_success) {
                logger().success("Sent '" + std::string(buf, offset - 1) +
                                 "' of size " + std::to_string(buffer_size));
            }
            return 0;
        }
    }
}

int fd_wrapper::simple_receive_message(char *buf, size_t &buffer_size, unsigned int real_size, bool log_success) {
    if (!check_valid()) {
        logger().fail("Socket descriptor is invalid, can not receive message");
        return -1;
    }
    size_t offset = 0;
    while (true) {
        int status = receive_next(buf, buffer_size, real_size, offset);
        if (status < 0) {
            return -1;
        } else if (status == 0) {
            if (log_success) {
                logger().success("Received '" + std::string(buf) + "' of size " + std::to_string(offset));
            }
            return 0;
        }
    }
}
