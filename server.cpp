//
// Created by roman on 20.05.19.
//

#include "server.h"
#include "my_error.h"
#include <string>
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

server::server(): tcp_socket(socket(AF_INET, SOCK_STREAM, 0)) {
    memset(buffer, 0, BUFFER_SIZE + 1);
    if (tcp_socket == -1) {
        my_error("Socket cannot be created");
        throw runtime_error("Socket cannot be created");
    }
}

void server::start(const char *hostAddress, const in_port_t port) {
    struct sockaddr_in server{};
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    server.sin_port = port;

    if (inet_aton(hostAddress, &server.sin_addr) == 0) {
        my_error("Internet host address is invalid");
        throw runtime_error("Internet host address is invalid");
    }

    if (bind(tcp_socket, reinterpret_cast<sockaddr *>(&server), sizeof(sockaddr_in)) == -1) {
        my_error("Cannot bind");
        throw runtime_error("Cannot bind");
    }

    if (listen(tcp_socket, 32) == -1) {
        my_error("Cannot start listening the socket");
        throw runtime_error("Cannot start listening the socket");
    }
    my_fd epoll_fd(epoll_create(1));
    if (epoll_fd == -1) {
        my_error("Cannot create epoll instance");
        throw runtime_error("Cannot create epoll instance");
    }
    ev.events = EPOLLIN;
    ev.data.fd = tcp_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_socket, &ev)) {
        my_error("Cannot perform control operations on the epoll instance");
        throw runtime_error("Cannot perform control operations on the epoll instance");
    }
    while (true) {
        int ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (ready == -1) {
            my_error("epoll_wait problem");
            throw runtime_error("epoll_wait problem");
        }
        for (int i = 0; i < ready; ++i) {
            if (events[i].data.fd == tcp_socket) {
                struct sockaddr_in client{};
                size_t sockaddr_size = sizeof(struct sockaddr_in);
                int client_fd = accept(tcp_socket, reinterpret_cast<sockaddr *>(&client),
                                       reinterpret_cast<socklen_t *>(&sockaddr_size));
                if (client_fd == -1) {
                    my_error("Cannot accept client");
                    //do not drop server
                    continue;
                }
                std::cout << "[INFO] Client connected" << std::endl;
                int flags = fcntl(client_fd, F_GETFL, 0);
                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    my_error("Cannot invoke epoll_ctl on this client");
                    if (close(client_fd) == -1) {
                        my_error("Cannot close connection");
                    }
                }
            } else {
                int client_fd = events[i].data.fd;
                ssize_t readed = read(client_fd, buffer, BUFFER_SIZE); // yeah, english vse dela read read
                if (readed <= 0) {
                    if (readed == -1) {
                        my_error("Cannot read request");
                    }
                    std::cout << "[INFO] Client disconnected" << std::endl;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr) == -1) {
                        if (close(client_fd) == -1) {
                            my_error("Cannot close connection");
                        }
                    }
                    continue;
                }
                std::cout << "[REQUEST] " << std::string(buffer, readed) << std::endl;
                ssize_t sent = send(client_fd, buffer, readed, 0);
                if (sent != readed) {
                    cerr << "[ERROR] Cannot send full response" << std::endl;
                    if (sent == -1) {
                        my_error("Error during sending response");
                    }
                }
            }
        }
    }
}
