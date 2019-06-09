#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <iostream>

#include "server.hpp"

server::server(const std::string &address, const std::string &port) {
    check_ipv4(address);
    create_socket();
    set_socket_options(port);
    bind_to_address();
}

void server::wait_clients() {
    if (listen(server_fd, 10) < 0) {
        detach(server_fd);
        print_fatal_error("listen");
    }
    accept_connection();
}

void server::check_ipv4(const std::string &address) {
    struct sockaddr_in sa;
    if (inet_pton(AF_INET, address.c_str(), &(sa.sin_addr)) == 0) {
        std::cerr << "server: invalid ipv4 address" << std::endl;
        throw std::runtime_error("invalid ipv4 address");
    }
}

void server::create_socket() {
    server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd == -1) {
        perror("socket failed");
    }
    std::cout << "socket: " << server_fd << std::endl;
}

void server::set_socket_options(const std::string &port) {
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof opt) == -1) {
        detach(server_fd);
        print_fatal_error("setsockopt");
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(port.c_str()));
}

void server::bind_to_address() {
    if (bind(server_fd, (struct sockaddr *)&server_address,
             sizeof server_address) < 0) {
        detach(server_fd);
        print_fatal_error("bind failed");
    }
}

void server::accept_connection() {
    struct epoll_event event, events[max_events];
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_fd = epoll_create(1);
    if (epoll_fd < 0) {
        print_fatal_error("epoll");
    }
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        print_fatal_error("can't add socket descriptor to epoll");
    }
    for (;;) {
        int waiting_clients = epoll_wait(epoll_fd, events, max_events, -1);
        if (waiting_clients < 0) {
            print_fatal_error("waiting for clients failed");
        }
        for (int i = 0; i < waiting_clients; ++i) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client;
                size_t socket_size = sizeof(struct sockaddr_in);
                int client_fd = accept(server_fd, (struct sockaddr *)&client,
                                       (socklen_t *)&socket_size);
                if (client_fd < 0) {
                    perror("accept failed");
                    continue;
                }
                int flags = fcntl(client_fd, F_GETFL, 0);
                flags |= O_NONBLOCK;
                fcntl(client_fd, F_SETFL, flags);
                event.data.fd = client_fd;
                event.events = EPOLLIN | EPOLLOUT;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
                    perror("adding socket descriptor to epoll failed");
                    // TODO: add descriptor class
                    if (close(client_fd) < 0) {
                        perror("can't close descriptor");
                    }
                }
            }
            if ((events[i].events & EPOLLIN) && (events[i].events & EPOLLOUT)) {
                std::array<char, 1024> buffer;
                int message_size =
                        recv(events[i].data.fd, buffer.data(), buffer.size(), 0);
                if (message_size < 0) {
                    perror("receiving failed");
                    continue;
                }
                std::cout << buffer.data() << std::endl;
                buffer[message_size] = '\0';
                std::string message = "hello, client";
                if (send(events[i].data.fd, message.data(), message.size(), 0) < 0) {
                    perror("server: sending failed");
                    continue;
                }
                event.data.fd = events[i].data.fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &event) < 0) {
                    perror("can't remove client descriptor from epoll");
                }
                if (close(events[i].data.fd) < 0) {
                    perror("can't close client descriptor");
                }
            }
        }
    }
}

void server::detach(int fd) {
    if (close(fd) < 0) {
        perror("Can't close socket");
    }
}

void server::print_fatal_error(const std::string &err) {
    perror(err.c_str());
    throw std::runtime_error("server: " + err);
}
