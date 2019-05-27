#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

#include "Server.h"

Server::Server(uint16_t port) : port(port),
                                listenfd(socket(AF_INET, SOCK_STREAM, 0 /* or IPPROTO_TCP*/)),
                                epollfd(epoll_create1(0)) {

    struct sockaddr_in socket_addr;

    memset(&socket_addr, 0, sizeof(struct sockaddr_in));
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_addr.s_addr = INADDR_ANY;
    socket_addr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *) &socket_addr, sizeof(sockaddr_in)) == -1) {
        throw ServerException("Bind failed");
    }

    if (listen(listenfd, LISTEN_BACKLOG) == -1) {
        throw ServerException("Listen failed");
    }

    if (!epoll_ctl_add(epollfd, listenfd, EPOLLIN)) {
        throw ServerException("Epoll_ctl failed");
    }

    if (!epoll_ctl_add(epollfd, 0, EPOLLIN)) {
        throw ServerException("I wanted to listen to stdin :(");
    }
}

void Server::closeFileDescriptor(int fd) {
    if (close(fd) == -1) {
        perror("File descriptor was not closed");
    }
}

bool Server::epoll_ctl_add(int efd, int fd, uint32_t ev) {
    epollEvent.events = ev;
    epollEvent.data.fd = fd;

    return epoll_ctl(efd, EPOLL_CTL_ADD, fd, &epollEvent) != -1;
}

void Server::run() {
    while (!finished) {
        const int fdNumber = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (fdNumber == -1) {
            throw ServerException("Wait failed");
        }
        for (int i = 0; i < fdNumber; i++) {
            if (events[i].data.fd == listenfd) {
                struct sockaddr_in addr;
                socklen_t addrLen = sizeof(addr);

                const int sfd = accept(listenfd, (struct sockaddr *) &addr, &addrLen);
                if (sfd == -1) {
                    perror("Connection failed");
                    continue;
                }
                std::cout << "[S] Accept OK" << std::endl;
                const int flags = fcntl(sfd, F_GETFL, 0);
                fcntl(sfd, F_SETFL, flags | O_NONBLOCK);

                if (!epoll_ctl_add(epollfd, sfd, EPOLLIN | EPOLLET)) {
                    perror("Failed to register fd");
                    closeFileDescriptor(sfd);
                }
            } else if (events[i].data.fd == 0) {
                readStdin();
            } else {
                processRequest(events[i].data.fd);
            }
        }
    }
}

void Server::readStdin() {
    std::string line;
    std::getline(std::cin, line);
    if (line == "exit") {
        finished = true;
    }
}

void Server::processRequest(int fd) {
    ssize_t len = read(fd, requestBuffer, BUFFER_SIZE);
    if (len == -1) {
        perror("Reading failed");
        disconnect(fd);
    } else if (len == 0) {
        disconnect(fd);
    } else {
        sendReply(fd, len);
    }
}

void Server::disconnect(int fd) {
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        perror("Disconnect failed");
    }
}

void Server::sendReply(int fd, int len) {
    std::cout << "[C" + std::to_string(fd) + "] " << std::string(requestBuffer, len) << std::endl;
    if (send(fd, requestBuffer, len, 0) == -1) {
        perror("Response was not sent");
        disconnect(fd); // Maybe it's a bad plan
    }
}
