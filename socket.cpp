//
// Created by anastasia on 09.06.19.
//
#ifndef OS_NET_MULTIPLEXING_SOCKET_H
#define OS_NET_MULTIPLEXING_SOCKET_H


#include <netinet/in.h>
#include <string>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <iostream>

struct socket_exception : std::runtime_error {
    explicit socket_exception(const std::string &cause)
            : std::runtime_error(cause + ": " + strerror(errno)){}
};


class Socket {
    int fd;
    int flags{};
    static const int MAX = 500;
    static const int BUFFER_SIZE = 2048;
public:
    Socket() : fd(socket(AF_INET, SOCK_STREAM, 0)) {
        if (fd == -1) {
            throw socket_exception("Can't create socket");
        }
        flags = fcntl(fd, F_GETFL);
    }

    Socket(Socket &&other) noexcept : fd(other.fd) {
        other.fd = -1;
        flags = other.flags;
        other.flags = 0;
    }

    explicit Socket(int fd) : fd(fd) {}

    Socket &operator=(Socket &&other) noexcept {
        fd = other.fd;
        other.fd = -1;
        flags = other.flags;
        other.flags = 0;
        return *this;
    }

    void connect(sockaddr_in server_addr) {
        if (::connect(fd, reinterpret_cast<const sockaddr *>(&server_addr), sizeof(sockaddr_in)) == -1 && !(flags & SOCK_NONBLOCK)) {
            throw socket_exception("Can't connect to server");
        }
    }

    void send(std::string text) {
        size_t writen_size = 0;
        while (writen_size != text.length() + 1) {
            ssize_t cur = ::send(fd, text.c_str() + writen_size, text.length() + 1 - writen_size, 0);
            if (cur == -1) {
                throw socket_exception("Can't send request");
            }
            writen_size += cur;
        }
    }

    std::string recv() {
        std::vector<char> ans((unsigned long) BUFFER_SIZE);
        size_t readen_size = 0;
        while (readen_size == 0 || ans[readen_size - 1] != '\0') {
            ssize_t sz = ::recv(fd, ans.data() + readen_size, (size_t) BUFFER_SIZE, 0);
            if (sz == -1) {
                throw socket_exception("Can't read from socket");
            }
            readen_size += sz;
        }
        ans.resize(static_cast<unsigned long>(readen_size));
        return std::string(ans.data());
    }

    void bind(sockaddr_in server_addr) {
        if (::bind(fd, reinterpret_cast<const sockaddr *>(&server_addr), sizeof(sockaddr)) == -1) {
            throw socket_exception("Can't bind server");
        }
    }

    void listen(int connections) {
        if (::listen(fd, connections) == -1) {
            throw socket_exception("Can't listen socket");
        }
    }

    Socket accept() {
        sockaddr_in addr{};
        socklen_t  addr_size;
        int connection(::accept(fd, nullptr, nullptr));
        if (connection == -1) {
            throw socket_exception("Can't accept connection");
        }
        return Socket(connection);
    }

    int getFd() {
        return fd;
    }

    void unblock() {
        if (flags & O_NONBLOCK)
            return;
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw socket_exception("Can't make socket nonblock");
        }
        flags |= O_NONBLOCK;
    }

    void close() {
        if (fd == -1)
            return;
        if (::close(fd) == -1) {
            std::cerr << "Can't close socket" << std::endl;
        }
    }

};


#endif //OS_NET_MULTIPLEXING_SOCKET_H
