#include "socket_wrapper.h"

ssocket::ssocket() : descriptor(socket(AF_INET, SOCK_STREAM, 0)) {
    if (descriptor == -1) {
        throw socket_exception("Cannot create a socket");
    }
    flags = fcntl(descriptor, F_GETFL);
    if (flags == -1) {
        throw socket_exception("Cannot get socket status");
    }
}

ssocket::ssocket(int fd) : descriptor(fd) {}

ssocket::ssocket(ssocket&& other) noexcept : descriptor(other.descriptor) {
    other.descriptor = -1;
}

ssocket& ssocket::operator=(ssocket&& other) noexcept {
    descriptor = other.descriptor;
    other.descriptor = -1;
    return *this;
}

void ssocket::bind(sockaddr_in& address) {
    if (::bind(descriptor, reinterpret_cast<sockaddr*>(&address),
               sizeof(address)) == -1) {
        throw socket_exception("Cannot bind socket");
    }
}

void ssocket::listen(int connections) {
    if (::listen(descriptor, connections) == -1) {
        throw socket_exception("Cannot set socket to listening state");
    }
}

ssocket ssocket::accept() {
    int new_descriptor = ::accept(descriptor, nullptr, nullptr);
    if (descriptor == -1) {
        throw socket_exception("Cannot accept new connection");
    }
    return ssocket(new_descriptor);
}

int ssocket::connect(sockaddr_in& address) {
    if (::connect(descriptor, reinterpret_cast<sockaddr*>(&address),
                  sizeof(address)) == -1) {
        if (flags & SOCK_NONBLOCK) {
            return -1;
        } else {
            throw socket_exception("Cannot connect to server");
        }
    }
    return 0;
}

size_t ssocket::send(std::string data) {
    ssize_t was_sent = ::send(descriptor, data.data(), data.size(), 0);
    if (was_sent == -1) {
        throw socket_exception("Error while sending");
    }
    return static_cast<size_t>(was_sent);
}

std::string ssocket::read() {
    std::vector<char> buffer(BUFF_SIZE);
    ssize_t was_read = recv(descriptor, buffer.data(), BUFF_SIZE, 0);
    if (was_read == -1) {
        throw socket_exception("Cannot recieve data from socket");
    }
    buffer.resize(static_cast<size_t>(was_read));
    return std::string(buffer.data());
}

int ssocket::get_desc() { return descriptor; }

void ssocket::unblock() {
    if (!(flags & O_NONBLOCK)) {
        if (::fcntl(descriptor, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw socket_exception("Cannot change socket status");
        }
        flags |= O_NONBLOCK;
    }
}

ssocket::~ssocket() {
    if (descriptor != -1) {
        if (close(descriptor) == -1) {
            std::cerr << "Cannot close socket's file descriptor" << std::endl;
        }
    }
}
