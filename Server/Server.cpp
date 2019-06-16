//
// Created by Павел Пономарев on 2019-05-27.
//

#include "Server.h"
#include "ServerException.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <iostream>
#include <stdlib.h>
#include <cstring>


Server::Server(int port) : mPort(port) {
    mListener = openSocket();
    mEpoll = epoll_create(POLL_SIZE); // open an epoll file descriptor
    if (!bindSocket()) {
        throw ServerException(getMessage("Bind failed"));
    }
    if (listen(mListener, BACKLOG) == -1) {
        throw ServerException(getMessage("Listen failed"));
    }
    if (!addToPoll(mEpoll, mListener, EPOLLIN)) {
        throw ServerException(getMessage("Epoll_ctl failed"));
    }
    if (!addToPoll(mEpoll, 0, EPOLLIN)) {
        throw ServerException("sorry");
    }

}

void Server::run() {
    while (true) {
        int ready = epoll_wait(mEpoll, events, POLL_SIZE, -1);
        if (ready == -1) {
            throw ServerException(getMessage("Error occurred while waiting"));
        }
        for (size_t i = 0; i < ready; ++i) {
            if (events[i].data.fd == mListener) {
                struct sockaddr_in addr{};
                socklen_t addrLen = sizeof(addr);
                int client = accept(mListener, (struct sockaddr*) &addr, &addrLen);
                if (client < 0) {
                    perror("Connection failed");
                    continue;
                }
                std::cout << "Connected successfully" << std::endl;
                int flags = fcntl(client, F_GETFL, 0);
                fcntl(client, F_SETFL, flags | O_NONBLOCK);

                if (!addToPoll(mEpoll, client, EPOLLIN | EPOLLET)) {
                    perror("Failed to register fd");
                    if (close(client) < 0) {
                        perror("Descriptor was not closed");
                    }
                }

            } else if (events[i].data.fd == 0) {
                std::string line;
                std::getline(std::cin, line);
                if (line == "quit") {
                    return;
                }
            } else {
                process(events[i].data.fd);
            }
        }
    }
}

int Server::openSocket() {
    int listener;
    listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listener < 0) {
        throw ServerException(getMessage("Failed to create socket"));
    }
    int opt = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        throw ServerException(getMessage("Failed to set socket options"));
    }
    return listener;
}

bool Server::bindSocket() {
    sockaddr_in addr{};
    addr.sin_port = htons(mPort);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    return bind(mListener, (struct sockaddr*) &addr, sizeof(addr)) >= 0;
}

bool Server::addToPoll(int efd, int fd, uint32_t ev) {
    event.events = ev;
    event.data.fd = fd;
    return (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event) >= 0);
}

void Server::process(int fd) {
    ssize_t length = read(fd, requestBuffer, BUFFER_SIZE);
    switch (length) {
        case 0: {
            if (epoll_ctl(mEpoll, EPOLL_CTL_DEL, fd, NULL) < 0) {
                perror("Epoll_ctl failed");
            }
            break;
        }
        case -1: {
            perror("Read failed");
            if (epoll_ctl(mEpoll, EPOLL_CTL_DEL, fd, NULL) < 0) {
                perror("Epoll_ctl failed");
            }
            break;
        }
        default: {
            sendResponse(fd, length);
        }
    }
}

void Server::sendResponse(int fd, ssize_t length) {
    std::cout << fd << " " << std::string(requestBuffer, length) << std::endl;
    if (write(fd, requestBuffer, length) < 0) {
        perror("Write failed");
    }
}

std::string Server::getMessage(std::string const& message) {
    return std::string(message) + ": " + std::strerror(errno);
}

Server::~Server() {
    if (close(mListener) < 0) {
        perror("Descriptor (listener) was not closed");
    }
    if (close(mEpoll) < 0) {
        perror("Descriptor (epoll) was not closed");
    }

}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "incorrect number of arguments";
        return EXIT_FAILURE;
    }
    int port = atoi(argv[1]);
    try {
        Server server(port);
        server.run();
    } catch (ServerException& e) {
        std::cout << e.what();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}