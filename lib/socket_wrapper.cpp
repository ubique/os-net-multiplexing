//
// Created by vitalya on 27.05.19.
//

#include "socket_wrapper.h"
#include "error.h"


socket_wrapper::socket_wrapper()
    : fd(socket(AF_INET, SOCK_STREAM, 0))
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
{
    other.fd = -1;
}

socket_wrapper::~socket_wrapper() {
    if (fd != -1) {
        close(fd);
    }
}

socket_wrapper& socket_wrapper::operator=(socket_wrapper&& other) noexcept {
    //name = std::move(other.name);
    fd = other.fd;
    other.fd = -1;
    return *this;
}

void socket_wrapper::bind(sockaddr_in& address) {
    int ret = ::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    if (ret == -1) {
        error("Unable to bind socket");
    }
}

void socket_wrapper::listen() {
    int ret = ::listen(fd, 10);
    if (ret == -1) {
        error("Unable to listen socket");
    }
}

void socket_wrapper::connect(sockaddr_in& address) {
    int ret = ::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr));
    if (ret == -1) {
        error("Unable to connect socket");
    }
}

socket_wrapper socket_wrapper::accept() {
    if (fd == -1) {
        error("Unable to accept socket");
    }
    int new_fd = ::accept(fd, nullptr, nullptr);
    return socket_wrapper(new_fd);
}

int socket_wrapper::read(char* buffer, int size) {
    int len = ::read(fd, buffer, size);
    if (len == -1) {
        error("Unable to read data from socket");
    }
    return len;
}

std::string socket_wrapper::readMessage() {
    size_t BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE + 1];

    std::string result;
    while (true) {
        int len = ::read(fd, buffer, BUFFER_SIZE);
        if (len == -1) {
            error("Unable to read data from socket");
        }
        buffer[BUFFER_SIZE] = 0;
        if (!len)
            continue;
        result.append(buffer, len);
        if (buffer[len - 1] == 0) {
            result.pop_back();
            break;
        }
    }
    return result;
}

void socket_wrapper::write(const std::string& message) {
    int ret = ::write(fd, message.data(), message.size());
    if (ret == -1) {
        error("Unable to write data to socket");
    }
}

void socket_wrapper::writeMessage(const std::string& message) {
    std::string msg(message);
    msg += '\0';
    size_t ptr = 0;
    while (ptr < msg.size()) {
        int len = ::write(fd, msg.c_str() + ptr, msg.size() - ptr);
        if (len == -1) {
            error("Unable to write data to socket");
        }
        ptr += len;
    }
}

int socket_wrapper::get_fd() {
    return fd;
}

