//
// Created by Михаил Терентьев on 2019-05-23.
//


#include <string>
#include <vector>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include "client.h"
#include <sys/epoll.h>
#include "client_exception.h"


client::client(std::string const &address, std::string const &port_representation) : socket_fd(
        socket(AF_INET, SOCK_STREAM, 0)), epollfd(epoll_create1(0)) {
    int port = stoi(port_representation);
    if (socket_fd.isBroken()) {
        throw client_exception("Unable to create socket");
    }
    if (epollfd.isBroken()) {
        throw client_exception("Unable to create epoll");
    }
    insrt_epoll(STDIN_FILENO, EPOLLIN);
    fillAddress(address, port);

    ssize_t flags = fcntl(socket_fd.getDescriptor(), F_GETFL);
    if (flags == -1) {
        throw client_exception("Unable to get socket status");
    }
    if (fcntl(socket_fd.getDescriptor(), F_SETFL, flags | O_NONBLOCK) == -1) {
        throw client_exception("Unable to change socket mode");
    }

    if (connect(socket_fd.getDescriptor(), reinterpret_cast<sockaddr *>(&server), sizeof(sockaddr_in)) == -1) {
        if (errno != EINPROGRESS) {
            throw client_exception("Unable to connect to the server");
        }
    } else {
        connected = true;
        insrt_epoll(socket_fd.getDescriptor(), EPOLLIN);
    }
}

void client::run() {
    std::string message;
    std::string request_buffer;
     isInterrupted = false;
    std::cout << "Client is running" << std::endl;
    while (!isInterrupted) {
        request_buffer = "";
        int dcnt = epoll_wait(epollfd.getDescriptor(), events, MAX_EVENTS, -1);
        if (dcnt == -1) {
            throw client_exception("epoll_wait  error");
        }

        for (int i = 0; i < dcnt; ++i) {
            if (!connected) {
                int status = 0;
                socklen_t s = sizeof(socket_fd.getDescriptor());
                if (getsockopt(socket_fd.getDescriptor(), SOL_SOCKET, SO_ERROR, &status, &s) == 0) {
                    connected = true;
                    insrt_epoll(socket_fd.getDescriptor(), EPOLLIN);
                }
            }
            if (events[i].data.fd == socket_fd.getDescriptor()) {
                ssize_t recv_status = recv(socket_fd.getDescriptor(), server_reply, BF_SZ, 0);
                if (recv_status < 0) {
                    throw client_exception("Receiving error");
                } else if (recv_status == 0) {
                    isInterrupted = true;
                    continue;
                }
                std::cout << std::string{server_reply, static_cast<size_t>(recv_status)} << std::endl;
            }
            if (events[i].data.fd == 0) {
                getline(std::cin, message);
                if (message.empty() || !std::cin) {
                    isInterrupted = true;
                    break;
                }
                if (!message.empty()) {
                    request_buffer += message + "\n";
                }

                if (!request_buffer.empty() && connected) {
                    send(socket_fd.getDescriptor(),request_buffer);
                    request_buffer = "";
                }
            }
        }
    }
}

void client::insrt_epoll(int fd, uint32_t events) {
    epoll_event event;
    event.data.fd = fd;
    event.events = events;

    if (epoll_ctl(epollfd.getDescriptor(), EPOLL_CTL_ADD, fd, &event) == -1) {
        throw client_exception("Unable to add descriptor to epoll");
    }
}
void client::send(int desc, std::string const& message) {
    ssize_t was_sent = 0;
    size_t tries_to_send = 50;
    while (was_sent < message.size() && tries_to_send--) {
        ssize_t cnt = ::write(socket_fd.getDescriptor(), message.data() + was_sent, static_cast<size_t>(message.size() - was_sent));
        if (cnt != -1) {
            was_sent += cnt;
        }
    }
    if (was_sent == 0) {
        isInterrupted = true;
        throw client_exception("Couldn't send request to socket " + std::to_string(desc));
    }
}

void client::fillAddress(std::string const &address, int port) {
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(address.data());
    server.sin_port = htons(static_cast<uint16_t>(port));
}