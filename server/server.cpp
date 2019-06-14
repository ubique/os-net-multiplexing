//
// Created by vitalya on 19.05.19.
//

#include "server.h"

#include <cstring>
#include <zconf.h>
#include <iostream>


const int server::MAX_EVENTS = 100;

server::server(char* addr, int port)
    : events(new epoll_event[MAX_EVENTS])
    , server_address(addr)
{
    memset(&address, 0, sizeof(sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = port;
    address.sin_addr.s_addr = inet_addr(addr);

    socket_wrapper server_socket = socket_wrapper();
    server_socket.bind(address);
    server_socket.listen();
    server_fd = server_socket.get_fd();

    epoll.start();
    epoll.process(server_fd, EPOLLIN, EPOLL_CTL_ADD);

    sockets[server_fd] = std::move(server_socket);
}

void server::start() {
    std::cout << "Echo server started!\n" << std::endl;

    while (!down_flag) {
        int events_number = epoll.wait(events.get(), MAX_EVENTS);

        for (int i = 0; i < events_number; ++i) {
            uint32_t tmp_events = events.get()[i].events;
            int tmp_fd = events.get()[i].data.fd;

            if (tmp_events & (EPOLLERR | EPOLLHUP)) {
                epoll.process(tmp_fd, -1, EPOLL_CTL_DEL);
                std::cout << "Client disconnected" << std::endl;
                if (responses.count(tmp_fd)) {
                    responses.erase(tmp_fd);
                }
                if (sockets.count(tmp_fd)) {
                    sockets.erase(tmp_fd);
                }
                continue;
            }
            if (tmp_events & EPOLLIN) {
                if (tmp_fd == server_fd) {
                    socket_wrapper client_socket = sockets[server_fd].accept();
                    client_socket.unblock();
                    epoll.process(client_socket.get_fd(), EPOLLIN | EPOLLERR | EPOLLHUP, EPOLL_CTL_ADD);
                    sockets[client_socket.get_fd()] = std::move(client_socket);
                    std::cout << "Client connected" << std::endl;
                } else {
                    if (responses[tmp_fd].empty()) {
                        epoll.process(tmp_fd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP, EPOLL_CTL_MOD);
                    }
                    responses[tmp_fd].push_back(sockets[tmp_fd].readMessage());
                }
            }
            if (tmp_events & EPOLLOUT) {
                sockets[tmp_fd].writeMessage(responses[tmp_fd].front());
                responses[tmp_fd].pop_front();
                if (responses[tmp_fd].empty()) {
                    responses.erase(tmp_fd);
                    epoll.process(tmp_fd, EPOLLIN | EPOLLERR | EPOLLHUP, EPOLL_CTL_MOD);
                }
            }
        }
    }
}

void server::stop() {
    down_flag = true;
}
