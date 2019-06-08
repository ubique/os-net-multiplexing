//
// Created by daniil on 08.06.19.
//

#include <arpa/inet.h>
#include <iostream>
#include "Client.h"

Client::Client(char *address, uint16_t port) : client_socket(Socket()) {
    sockaddr_in sockaddrIn;
    memset(&sockaddrIn, 0, sizeof(sockaddr_in));
    sockaddrIn.sin_port = port;
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_addr.s_addr = inet_addr(address);
    try {
        client_socket.connect(sockaddrIn);
    } catch (socket_exception &e) {
        throw client_error(e.what());
    }
    try {
        epoll.start();
        epoll.doEvent(client_socket.getDescriptor(), EPOLLIN, EPOLL_CTL_ADD);
        epoll.doEvent(0, EPOLLIN, EPOLL_CTL_ADD);
    } catch (epoll_exception& e) {
        throw client_error(e.what());
    }
}

void Client::run() {
    struct epoll_event events[EPOLL_SIZE];
    while (true) {
        try {
            int num = epoll.wait(events);
            for (int i = 0; i < num; i++) {
                if (events[i].data.fd == client_socket.getDescriptor()) {
                    try {
                        std::string response = client_socket.read();
                        std::cout << "Echo: " << response << "\n";
                    } catch (socket_exception& e) {
                        throw client_error(e.what());
                    }
                } else if (events[i].data.fd == 0) {
                    std::string message;
                    std::cin >> message;
                    if (message == "exit") break;
                    try {
                        client_socket.send(message);
                    } catch (socket_exception& e) {
                        throw client_error(e.what());
                    }
                }
            }
        } catch (std::runtime_error& e) {
            throw client_error(e.what());
        }
    }
}

