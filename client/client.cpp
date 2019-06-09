//
// Created by vitalya on 19.05.19.
//

#include "client.h"

#include <arpa/inet.h>
#include <cstring>
#include <deque>
#include <zconf.h>
#include <iostream>
#include <cmath>

const int client::MAX_EVENTS = 3;

client::client(char* addr, int port)
    : events(new epoll_event[MAX_EVENTS])
    , server_address(addr)
    , client_socket()
{
    memset(&address, 0, sizeof(sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = port;
    address.sin_addr.s_addr = inet_addr(addr);
    client_socket.connect(address);

    std::cout << "Connected to server" << std::endl;

    epoll.start();
    epoll.process(STDIN_FILENO, EPOLLIN, EPOLL_CTL_ADD);
    epoll.process(client_socket.get_fd(), EPOLLIN, EPOLL_CTL_ADD);
}

void client::start() {
    while (!down_flag) {
        int events_number = epoll.wait(events.get(), MAX_EVENTS);

        for (int i = 0; i < events_number; ++i) {
            int tmp_fd = events.get()[i].data.fd;

            if (tmp_fd == client_socket.get_fd()) {
                std::string response = client_socket.readMessage();
                std::cout << "Received: " + response + "\n" << std::endl;
            } else  {
                std::string request;
                std::cin >> request;
                if (request == "exit" || std::cin.eof()) {
                    stop();
                    continue;
                }
                client_socket.writeMessage(request);
                std::cout << "Sent: " + request + "\n" << std::endl;
            }
        }
    }
}

void client::stop() {
    down_flag = true;
    epoll.stop();
}

