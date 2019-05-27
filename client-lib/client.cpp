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
    fillAddress(address, port);
    if (connect(socket_fd.getDescriptor(), reinterpret_cast<sockaddr *>(&server),
                sizeof(server)) == -1) {
        throw client_exception("Unable to connect to server");
    }
    if (epollfd.isBroken()) {
        throw client_exception("Unable to create epoll");
    }
    main_event.events = EPOLLIN;
    main_event.data.fd = socket_fd.getDescriptor();
    if (epoll_ctl(epollfd.getDescriptor(), EPOLL_CTL_ADD, socket_fd.getDescriptor(), &main_event) == -1) {
        throw client_exception("Unable to add server to epoll_ctl");
    }
    main_event.events = EPOLLIN;
    main_event.data.fd = 0;
    if (epoll_ctl(epollfd.getDescriptor(), EPOLL_CTL_ADD, 0, &main_event) == -1) {
        throw std::runtime_error("Unable to add stdin to epoll_ctl");
    }
}

void client::run() {
    std::string message;
    bool isInterrupted = false;
    std::cout << "Client is running" << std::endl;
    while (!isInterrupted) {
        int dcnt = epoll_wait(epollfd.getDescriptor(), events, MAX_EVENTS, -1);
        if (dcnt == -1) {
            throw client_exception("epoll_wait  error");
        }

        for (int i = 0; i < dcnt; ++i) {
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
                if (message.empty()) {
                    isInterrupted = true;
                    continue;
                }
                if (send(socket_fd.getDescriptor(), message.data(), message.size(), 0) == -1) {
                    throw client_exception("Sending request error");
                }
            }
        }
    }
}

void client::fillAddress(std::string const &address, int port) {
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(address.data());
    server.sin_port = htons(static_cast<uint16_t>(port));
}

