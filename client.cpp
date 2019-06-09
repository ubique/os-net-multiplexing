#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>

#include "client.hpp"

client::client(const std::string &address, const std::string &port) {
    create_socket();
    set_sock_options(address, port);
}

void client::connect_to_server() {
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
            0 &&
            errno != EINPROGRESS) {
        print_fatal_error("Connection Failed");
    }
    const std::string client_name = "kukarek"; // TODO: diff names for servers
    struct epoll_event event, events[max_events];
    event.events = EPOLLIN;
    event.data.fd = client_fd;
    epoll_fd = epoll_create(1);
    if (epoll_fd < 0) {
        print_fatal_error("epoll");
    }
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
        print_fatal_error("can't add socket descriptor to epoll");
    }
    int ready = epoll_wait(epoll_fd, events, max_events, -1);
    if (ready < 0) {
        print_fatal_error("waiting output failed");
    }
    int i = 0;
    for (; i < ready; ++i) {
        if (events[i].data.fd == client_fd && (events[i].events & EPOLLOUT)) {
            if (send(client_fd, client_name.data(), client_name.size(), 0) < 0) {
                print_fatal_error("sending failed");
            }
            break;
        }
    }
    if (i == ready) {
        print_fatal_error("Cannot send data to server");
    }
}

void client::work() {
    const std::string hello = "hello, server";
    send(client_fd, hello.c_str(), hello.size(), 0);
    std::cout << "client: sent" << std::endl;

    std::array<char, 1024> buffer;
    read(client_fd, buffer.data(), buffer.size());
    std::cout << buffer.data() << std::endl;
}

void client::create_socket() {
    if ((client_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
        perror("Socket creation error");
    }
}

void client::set_sock_options(const std::string &addr_to_connect,
                              const std::string &port) {
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(port.c_str()));

    if (inet_pton(AF_INET, addr_to_connect.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "client: invalid ipv4 address" << std::endl;
        throw std::runtime_error("invalid ipv4 address");
    }
}

void client::print_fatal_error(const std::string &err) {
    perror(err.c_str());
    throw std::runtime_error("client: " + err);
}
