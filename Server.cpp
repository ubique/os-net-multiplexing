//
// Created by daniil on 08.06.19.
//

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <map>
#include <vector>
#include <iostream>
#include "Server.h"
#include "Epoll.h"

Server::Server(const char *addres, uint16_t port) : server_socket(Socket()) {
    sockaddr_in sockaddrIn;
    memset(&sockaddrIn, 0, sizeof(sockaddr_in));
    sockaddrIn.sin_port = port;
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_addr.s_addr = inet_addr(addres);
    try {
        server_socket.bind(sockaddrIn);
        server_socket.listen(10);
    } catch (socket_exception &e) {
        throw server_error(e.what());
    }
    try {
        epoll.start();
        epoll.doEvent(server_socket.getDescriptor(), EPOLLIN, EPOLL_CTL_ADD);
    } catch (epoll_exception &e) {
        throw server_error(e.what());
    }
}

void Server::run() {
    struct epoll_event events[EVENTS_SIZE];
    std::map<int, std::vector<std::string>> responses;
    while (true) {
        try {
            int num = epoll.wait(events);
            for (int i = 0; i < num; i++) {
                int descriptor = events[i].data.fd;
                if (descriptor == server_socket.getDescriptor()) {
                    try {
                        Socket client_socket = server_socket.accept();
                        client_socket.unblock();
                        epoll.doEvent(client_socket.getDescriptor(), EPOLLIN | EPOLLERR | EPOLLHUP, EPOLL_CTL_ADD);
                        std::cout << "new client connected" << std::endl;
                    } catch (std::runtime_error &e) {
                        std::cerr << e.what() << "\n";
                        continue;
                    }
                } else {
                    Socket client_socket = Socket(events[i].data.fd);

                    if (events[i].events & (EPOLLHUP | EPOLLERR)) {
                        deleteSocket(client_socket);
                    } else if (events[i].events & EPOLLIN) {
                        std::string response = client_socket.read();
                        if (responses[client_socket.getDescriptor()].empty()) {
                            epoll.doEvent(client_socket.getDescriptor(), EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP,
                                          EPOLL_CTL_MOD);
                        }
                        responses[client_socket.getDescriptor()].push_back(response);
                    } else if (events[i].events & EPOLLOUT) {
                        client_socket.send(responses[client_socket.getDescriptor()].back());
                        responses[client_socket.getDescriptor()].pop_back();
                        if (responses[client_socket.getDescriptor()].empty()) {
                            epoll.doEvent(client_socket.getDescriptor(), EPOLLIN | EPOLLERR | EPOLLHUP, EPOLL_CTL_MOD);
                        }
                    }
                }
            }
        } catch (std::runtime_error &e) {
            deleteSocket(server_socket);
            throw server_error(e.what());
        }
    }
}

void Server::deleteSocket(Socket &socket) {
    try {
        epoll.doEvent(socket.getDescriptor(), -1, EPOLL_CTL_DEL);
        socket.close();
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
    }
}
