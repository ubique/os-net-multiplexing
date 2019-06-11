//
// Created by daniil on 08.06.19.
//

#include <arpa/inet.h>
#include <iostream>
#include <zconf.h>
#include "Client.h"
#include <vector>
#include <string>
#include <queue>

Client::Client(char *address, uint16_t port) : client_socket(Socket()) {
    sockaddr_in sockaddrIn;
    memset(&sockaddrIn, 0, sizeof(sockaddr_in));
    sockaddrIn.sin_port = port;
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_addr.s_addr = inet_addr(address);
    client_socket.unblock();
    try {
        client_socket.connect(sockaddrIn);
    } catch (socket_exception &e) {
        throw client_error(e.what());
    }
    try {
        epoll.start();
        epoll.doEvent(client_socket.getDescriptor(), EPOLLIN, EPOLL_CTL_ADD);
        epoll.doEvent(STDIN_FILENO, EPOLLIN, EPOLL_CTL_ADD);
    } catch (epoll_exception& e) {
        throw client_error(e.what());
    }

}

void Client::run() {
    struct epoll_event events[EPOLL_SIZE];
    std::queue<std::string> printable;
    std::queue<std::string> sendable;
    while (true) {
        try {
            int num = epoll.wait(events);
            for (int i = 0; i < num; i++) {
                if (events[i].data.fd == client_socket.getDescriptor()) {
                    if (events[i].events & EPOLLIN) {
                        try {
                            std::string response = client_socket.read();
                            if (printable.empty()) {
                                epoll.doEvent(STDOUT_FILENO, EPOLLOUT, EPOLL_CTL_ADD);
                            }
                            printable.push(response);
                        } catch (socket_exception &e) {
                            throw client_error(e.what());
                        }
                    } else if (events[i].events & EPOLLOUT) {
                        client_socket.send(sendable.front());
                        sendable.pop();
                        if (sendable.empty()) {
                            epoll.doEvent(events[i].data.fd, EPOLLIN, EPOLL_CTL_MOD);
                        }
                    }
                } else if (events[i].data.fd == STDIN_FILENO) {
                    std::string message;
                    std::cin >> message;
                    if (sendable.empty()) {
                        epoll.doEvent(client_socket.getDescriptor(), EPOLLIN | EPOLLOUT, EPOLL_CTL_MOD);
                    }
                    sendable.push(message);
                } else if (events[i].data.fd == STDOUT_FILENO) {
                    std::cout << "Echo: " << printable.front() << std::endl;
                    printable.pop();
                    if (printable.empty()) {
                        epoll.doEvent(STDOUT_FILENO, 0, EPOLL_CTL_DEL);
                    }
                }
            }
        } catch (std::runtime_error& e) {
            throw client_error(e.what());
        }
    }
}

