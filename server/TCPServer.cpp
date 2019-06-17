//
// Created by ifkbhit on 07.05.19.
//

#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include "TCPServer.h"

net::TCPServer::TCPServer(const std::string& ipv4, const std::string& port) : TCPSocket(ipv4,
                                                                                        port) {}

void net::TCPServer::run(const data_t&) {


    epoll_evt listen;
    listen.data.fd = getFileDescriptor();
    listen.events = EPOLLIN;
    epollCtl(EPOLL_CTL_ADD, &listen);

    while (true) {
        for (int i = 0; i < getDescNumber(); i++) {
            const int evt = evts[i].data.fd;
            if (evt == getFileDescriptor()) { // connecting
                struct sockaddr_in client{};
                socklen_t len = sizeof(client);
                int clientDescriptor = accept(evt, (struct sockaddr*) &client, &len);
                if (clientDescriptor == -1) {
                    perror("accept");
                    continue;
                }
                std::stringstream ss;
                ss << "Client " << std::string(inet_ntoa(client.sin_addr)) << ":"
                   << ntohs(client.sin_port); // uint16_t problem
                Utils::message(ss.str() + " connected");
                const int flags = fcntl(clientDescriptor, F_GETFL, 0);
                fcntl(clientDescriptor, F_SETFL, flags | O_NONBLOCK);

                epoll_evt clientEpoll;
                clientEpoll.data.fd = clientDescriptor;
                clientEpoll.events = EPOLLIN;

                epollCtl(EPOLL_CTL_ADD, &clientEpoll, clientDescriptor);
            } else { // echo
                data_t rec;
                if (read(evt, rec, -1) <= 0) {
                    Utils::error("Error while reading");
                    disconnect(evt);
                    continue;
                }
                int sent = send(evt, rec.data(), rec.size(), 0);
                if (sent == -1) {
                    perror("send echo");
                    disconnect(evt);
                }
                Utils::message("Echo of " + std::to_string(sent) + " bytes");
                Utils::message("");
                std::string input;
                std::cin >> input;
                if (input == "exit") {
                    return;
                }
            }
        }
    }
}

void net::TCPServer::disconnect(int fd) {
    Utils::message("Disconnect");
    if (epoll_ctl(epoll, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        perror("disconnect");
        exit(EXIT_FAILURE);
    }
}

bool net::TCPServer::afterCreate(const sockaddr_in& in) {
    if (bind(getFileDescriptor(), (struct sockaddr*) &in, sizeof(in)) < 0) {
        return Utils::fail("Socket bind (bind function)");
    }
    Utils::message("Server bind to " + fullAddress());
    if (listen(getFileDescriptor(), CONNECTION_QUEUE_SIZE) < 0) {
        return Utils::fail("Listen socket (listen function)");
    }
    Utils::message("Server created");
    return true;
}

