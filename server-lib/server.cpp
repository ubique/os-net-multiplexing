//
// Created by Михаил Терентьев on 2019-05-23.
//

#include "server.h"
#include "server_execption.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <vector>

#include <sys/types.h>
#include <sys/epoll.h>
#include <cstring>



server::server(std::string const &address, std::string const &port_representation) : socket_fd(socket(AF_INET, SOCK_STREAM, 0)),
                                                                        epoll_fd(epoll_create(1)) {
    int port = stoi(port_representation);
    if (socket_fd.isBroken()) {
        throw server_exception("Unable to create socket");
    }
    fillAddress(address,port);
    if (epoll_fd.isBroken()) {
        throw server_exception("Unable to create epoll instance");
    }

    add_to_epoll(socket_fd.getDescriptor(), EPOLLIN);

    if (bind(socket_fd.getDescriptor(), reinterpret_cast<sockaddr *>(&server_in), sizeof(server_in)) == -1) {
        throw server_exception("Unable to bind");
    }

    if (listen(socket_fd.getDescriptor(), BACKLOG_QUEUE_SIZE) == -1) {
        throw server_exception("Couldn't start listening to the socket");
    }
}

void server::run() {

    while (true) {
        int cnt = epoll_wait(epoll_fd.getDescriptor(), events, MAX_EVENTS, 500);

        if (cnt == -1) {
            continue;
        }

        for (int i = 0; i < cnt; i++) {
            uint32_t mode = events[i].events;
            int fd = events[i].data.fd;
            if(mode & EPOLLIN) {
                if(fd == socket_fd.getDescriptor()) {
                    wrapper client_fd(socket_fd.accept());
                    if(!client_fd.isBroken()) {
                        add_to_epoll(client_fd.getDescriptor(), EPOLLIN);
                        sockets[client_fd.getDescriptor()] = std::move(client_fd);
                    }
                } else {
                    send(fd, read(fd));
                }
            }
        }
    }
}


void server::fillAddress(std::string const &address,int port) {
    server_in.sin_family = AF_INET;
    server_in.sin_addr.s_addr = inet_addr(address.data());
    server_in.sin_port = htons(static_cast<uint16_t>(port));
}

void server::add_to_epoll(int sock_fd, uint32_t events) {
    epoll_event event;
    event.data.fd = sock_fd;
    event.events = events;

    if (epoll_ctl(epoll_fd.getDescriptor(), EPOLL_CTL_ADD, sock_fd, &event) == -1) {
        throw server_exception("Couldn't add descriptor to epoll");
    }

}

void server::remove_from_epoll(int sock_fd) {
    if (epoll_ctl(epoll_fd.getDescriptor(), EPOLL_CTL_DEL, sock_fd, nullptr) == -1) {
        throw server_exception("Couldn't remove descriptor from epoll");
    }

    sockets.erase(sock_fd);
}

std::string server::read(int desc) {
    std::vector<char> buffer;
    size_t tries_to_read = TRIES_NUMBER;
    std::string res;
    do {
        buffer.resize(BF_SZ);
        ssize_t stat = ::read(desc, buffer.data(), static_cast<size_t>(buffer.size()));
        if (stat == 0) {
            break;
        }
        if (stat != -1) {
            buffer.resize((size_t) stat);
            res.append(buffer.data());
        }
    } while (res.empty() && tries_to_read--);

    if (res.empty()) {
        remove_from_epoll(desc);
    }

    return res;
}

void server::send(int desc, std::string const& message) {
    ssize_t processed = 0;
    size_t tries_to_send = TRIES_NUMBER;
    while (processed < message.size() && tries_to_send--) {
        ssize_t stat = ::write(desc, message.data() + processed, static_cast<size_t>(message.size() - processed));
        if (stat != -1) {
            processed += stat;
        }
    }
    if (processed == 0) {
        remove_from_epoll(desc);
        throw server_exception("Unable to send respond to " + std::to_string(desc));
    }
}

