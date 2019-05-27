//
// Created by roman on 20.05.19.
//

#include <memory.h>
#include <arpa/inet.h>
#include "client.h"
#include "error.h"
#include <unistd.h>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>

using std::runtime_error;
using std::string;
using std::cin;
using std::cout;

client::client() {
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1) {
        error("Socket cannot be created");
    }
}


void client::connect_to(const char *hostAddress, const in_port_t port) {
    struct sockaddr_in server{};
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    server.sin_port = port;

    if (inet_aton(hostAddress, &server.sin_addr) == 0) {
        error("Internet host address is invalid");
    }

    if (connect(tcp_socket, reinterpret_cast<sockaddr *>(&server), sizeof(sockaddr_in)) == -1) {
        error("Cannot connect");
    }
}

void client::start() {
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        error("Cannot create epoll instance");
    }
    ev.events = EPOLLIN;
    ev.data.fd = tcp_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_socket, &ev) == -1) {
        error("Cannot invoke epoll_ctl on server");
    }
    ev.events = EPOLLIN;
    ev.data.fd = 0;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &ev) == -1) {
        error("Cannot add stdin");
    }
    bool flag = true;
    cout << "Type your name" << std::endl;
    while (flag) {
        int ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (ready == -1) {
            error("epoll_wait problem");
        }
        for (int i = 0; i < ready; ++i) {
            if (events[i].data.fd == 0) {
                string message;
                getline(cin, message);
                if (message == "STOP" || !cin) {
                    flag = false;
                    continue;
                }
                if (write(tcp_socket, message.data(), message.size() + 1) == -1) {
                    error("Cannot send message");
                }
            }
            if (events[i].data.fd == tcp_socket) {
                ssize_t readed = read(tcp_socket, buffer, BUFFER_SIZE);
                if (readed < 0) {
                    error("Error while receiving data from server");
                }
                if (readed == 0) {
                    flag = false;
                    continue;
                }
                cout << std::string(buffer) << std::endl;
            }
        }
    }

    if (close(tcp_socket) == -1 || close(epoll_fd) == -1)
        error("Cannot close socket");
}

