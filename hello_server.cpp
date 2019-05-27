//
// Created by roman on 20.05.19.
//

#include "hello_server.h"
#include "error.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <memory.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <sys/epoll.h>
#include <fcntl.h>

using std::runtime_error;
using std::cout;
using std::cerr;
using std::string;

hello_server::hello_server() {
    memset(buffer, 0, BUFFER_SIZE + 1);
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1) {
        error("Socket cannot be created");
    }
}

void hello_server::start(const char *hostAddress, const in_port_t port) {
    struct sockaddr_in server{};
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    server.sin_port = port;

    if (inet_aton(hostAddress, &server.sin_addr) == 0) {
        error("Internet host address is invalid");
    }

    if (bind(tcp_socket, reinterpret_cast<sockaddr *>(&server), sizeof(sockaddr_in)) == -1) {
        error("Cannot bind");
    }

    if (listen(tcp_socket, 32) == -1) {
        error("Cannot start listening the socket");
    }
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        error("Cannot create epoll instance");
    }
    ev.events = EPOLLIN;
    ev.data.fd = tcp_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_socket, &ev)) {
        error("Cannot perform control operations on the epoll instance");
    }
    while (true) {
        int ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (ready == -1) {
            error("epoll_wait problem");
        }
        for (int i = 0; i < ready; ++i) {
            if (events[i].data.fd == tcp_socket) {
                struct sockaddr_in client{};
                size_t sockaddr_size = sizeof(struct sockaddr_in);
                int client_fd = accept(tcp_socket, reinterpret_cast<sockaddr *>(&client),
                                       reinterpret_cast<socklen_t *>(&sockaddr_size));
                if (client_fd == -1) {
                    std::cerr << "Cannot accept client: " << strerror(errno) << std::endl;
                    //do not drop server
                    continue;
                }
                std::cout << "Client connected" << std::endl;
                int flags = fcntl(client_fd, F_GETFL, 0);
                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    std::cerr << "Cannot invoke epoll_ctl on this client: " << strerror(errno) << std::endl;
                    if (close(client_fd) == -1) {
                        std::cerr << "Cannot close connection: " << strerror(errno) << std::endl;
                    }
                }
            } else {
                int client_fd = events[i].data.fd;
                ssize_t readed = read(client_fd, buffer, BUFFER_SIZE);
                if (readed <= 0) {
                    if (readed == -1) {
                        std::cerr << "Cannot read request: " << strerror(errno) << std::endl;
                    }
                    std::cout << "Client disconnected" << std::endl;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr) == -1) {
                        if (close(client_fd) == -1) {
                            std::cerr << "Cannot close connection: " << strerror(errno) << std::endl;
                        }
                    }
                    continue;
                }
                std::cout << "REQUEST: " << std::string(buffer) << std::endl;
                make_response();
                size_t response_len = sizeof(response_len) + readed - 1;
                if (send(client_fd, buffer, response_len, 0) != response_len) {
                    cerr << "cannot send request" << std::endl;
                    //do not drop server
                }
            }
        }
    }
}


void hello_server::make_response() {
    memcpy(buffer + sizeof(response_prefix) - 1, buffer, BUFFER_SIZE);
    memcpy(buffer, response_prefix, sizeof(response_prefix) - 1);
}



