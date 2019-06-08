//
// Created by daniil on 27.05.19.
//

#include <fcntl.h>
#include <vector>
#include <zconf.h>
#include <iostream>
#include "Socket.h"

Socket::Socket() : fd(socket(AF_INET, SOCK_STREAM, 0)) {
    if (fd == -1) {
        throw socket_exception("Can't create socket");
    }
    flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        throw socket_exception("Can't get socket status");
    }
}

Socket &Socket::operator=(Socket &&other) noexcept {
    fd = other.fd;
    other.fd = -1;
    return *this;
}

Socket::Socket(Socket&& other) noexcept : fd(other.fd) {
    other.fd = -1;
}

void Socket::bind(sockaddr_in &address) {
    if (::bind(fd, reinterpret_cast<sockaddr *>(&address),
               sizeof(address)) == -1) {
        throw socket_exception("Can't bind socket");
    }
}

void Socket::listen(int connections) {
    if (::listen(fd, connections) == -1) {
        throw socket_exception("Can't set socket to listening state");
    }
}

Socket Socket::accept() {
    int new_descriptor = ::accept(fd, nullptr, nullptr);
    if (fd == -1) {
        throw socket_exception("Can't accept new connection");
    }
    return Socket(new_descriptor);
}

Socket::Socket(int descriptor) : fd(descriptor) {}

int Socket::connect(sockaddr_in &address) {
    if (::connect(fd, reinterpret_cast<sockaddr*>(&address),
                  sizeof(address)) == -1) {
        if (flags & SOCK_NONBLOCK) {
            return -1;
        } else {
            throw socket_exception("Can't connect to server");
        }
    }
    return 0;
}

int Socket::getDescriptor() {
    return fd;
}

size_t Socket::send(std::string &data) {
    size_t write_size = 0;
    while (write_size != data.size() + 1) {
        ssize_t cur_write_size = ::send(fd, data.c_str() + write_size, data.size() + 1 - write_size, 0);
        if (cur_write_size == -1) {
            throw socket_exception("Error while sending");
        }
        write_size += cur_write_size;
    }
    return static_cast<size_t>(write_size);
}

std::string Socket::read() {
    std::vector<char> buffer(BUFFER_SIZE);
    size_t read_size = 0;
    while (read_size == 0 || buffer[read_size - 1] != '\0') {
        ssize_t cur_read_size = ::recv(fd, buffer.data() + read_size, BUFFER_SIZE, 0);
        if (cur_read_size == -1) {
            throw socket_exception("Error while reading");
        }
        read_size += cur_read_size;
    }
    buffer.resize(read_size);
    return std::string(buffer.data());
}

void Socket::unblock() {
    if (!(flags & O_NONBLOCK)) {
        if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw socket_exception("Can't change socket status");
        }
        flags |= O_NONBLOCK;
    }
}

void Socket::close() {
    if (fd != -1) {
        if (::close(fd) == -1) {
            std::cerr << "Can't close socket" << std::endl;
        }
    }
}


