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
    //client_socket.unblock();
    client_socket.connect(address);

    std::cout << "Connected to server" << std::endl;

    epoll.start();
    epoll.process(STDIN_FILENO, EPOLLIN, EPOLL_CTL_ADD);
    epoll.process(client_socket.get_fd(), EPOLLIN, EPOLL_CTL_ADD);
}

void client::start() {
    std::deque<std::string> in, out;

    while (!down_flag) {
        int events_number = epoll.wait(events.get(), MAX_EVENTS);

        for (int i = 0; i < events_number; ++i) {
            uint32_t tmp_events = events.get()[i].events;
            int tmp_fd = events.get()[i].data.fd;

            if (tmp_fd == client_socket.get_fd()) {
                if (tmp_events & EPOLLIN) {
                    std::string response = client_socket.readMessage();
                    if (out.empty()) {
                        epoll.process(STDOUT_FILENO, EPOLLOUT, EPOLL_CTL_ADD);
                    }
                    out.push_back(response);
                } else if (tmp_events & EPOLLOUT) {
                    client_socket.writeMessage(in.front());
                    in.pop_front();
                    if (in.empty()) {
                        epoll.process(tmp_fd, EPOLLIN, EPOLL_CTL_MOD);
                    }
                }
            } else if (tmp_fd == STDIN_FILENO) {
                std::string request;
                std::cin >> request;
                if (in.empty()) {
                    epoll.process(client_socket.get_fd(), EPOLLIN | EPOLLOUT, EPOLL_CTL_MOD);
                }
                in.push_back(request);
            } else { // tmp_fd == STDOUT_FILENO
                std::cout << "Got: " + out.front() + "\n" << std::endl;
                out.pop_front();
                if (out.empty()) {
                    epoll.process(STDOUT_FILENO, 0, EPOLL_CTL_DEL);
                }
            }
        }
    }
}

