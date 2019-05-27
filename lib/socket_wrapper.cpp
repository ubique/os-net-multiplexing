//
// Created by vitalya on 27.05.19.
//

#include "socket_wrapper.h"
#include "lib/error.h"


socket_wrapper::socket_wrapper()
    : fd(socket(AF_UNIX, SOCK_SEQPACKET, 0))
{
    if (fd == -1) {
        error("Unable to create socket");
    }
}

socket_wrapper::socket_wrapper(int fd_)
    : fd(fd_)
{
}

socket_wrapper::socket_wrapper(socket_wrapper&& other) noexcept
    : fd(other.fd)
    , name(std::move(other.name))
{
    other.fd = -1;
}

socket_wrapper::~socket_wrapper() {
    if (fd != -1) {
        close(fd);
    }
}

socket_wrapper& socket_wrapper::operator=(socket_wrapper&& other) noexcept {
    name = std::move(other.name);
    fd = other.fd;
    other.fd = -1;
    return *this;
}

void socket_wrapper::create(const std::string& socket_name) {
    name = socket_name;
}

void socket_wrapper::bind(sockaddr_un& address) {
    int ret = ::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    if (ret == -1) {
        error("Unable to bind socket " + name);
    }
}

void socket_wrapper::listen() {
    int ret = ::listen(fd, 10);
    if (ret == -1) {
        error("Unable to listen socket " + name);
    }
}

void socket_wrapper::connect(sockaddr_un& address) {
    int ret = ::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr));
    if (ret == -1) {
        error("Unable to connect socket " + name);
    }
}

socket_wrapper socket_wrapper::accept() {
    if (fd == -1) {
        error("Unable to accept socket " + name);
    }
    int new_fd = ::accept(fd, nullptr, nullptr);
    return socket_wrapper(new_fd);
}

int socket_wrapper::read(char* buffer, int size) {
    int len = ::read(fd, buffer, size);
    if (len == -1) {
        error("Unable to read data from socket " + name);
    }
    return len;
}

void socket_wrapper::write(const std::string& message) {
    int ret = ::write(fd, message.data(), message.size());
    if (ret == -1) {
        error("Unable to write data to socket " + name);
    }
}

int socket_wrapper::get_fd() {
    return fd;
}

