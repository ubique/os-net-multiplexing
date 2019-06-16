#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <iostream>

#include "Client.h"

Client::Client(uint16_t port) : port(port),
                                sfd(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)),
                                epollfd(epoll_create1(0)) {
    epoll_ctl_add(epollfd, sfd, EPOLLIN);
    epoll_ctl_add(epollfd, 0, EPOLLIN);
}


void Client::closeFileDescriptor(int fd) {
    if (close(fd) == -1) {
        perror("File descriptor was not closed");
    }
}

void Client::run() {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(sfd, (struct sockaddr*) &addr, sizeof(addr)) == -1 && errno != EINPROGRESS) {
        throw ClientException("Connection failed");
    }

    std::string line;
    while (!finished) {
        const int fdNumber = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (fdNumber == -1) {
            throw ClientException("Wait failed");
        }
        for (int i = 0; i < fdNumber; i++) {
            if (events[i].data.fd == sfd) {
                getResponse();
            } else if (events[i].data.fd == 0) {
                checkStdin();
            }
        }
    }
}

void Client::getResponse() {
    ssize_t len;
    while (len = recv(sfd, responseBuffer, BUFFER_SIZE, 0)) {
        if (len == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            throw ClientException("Response was not received");
        } else {
            std::cout << "[S] " << std::string(responseBuffer, len) << std::endl;
        }
    }
}

void Client::checkStdin() {
    std::string line;
    std::getline(std::cin, line);
    if (line == "exit") {
        finished = true;
    } else {
        sendData(line);
    }
}

void Client::sendData(std::string const& data) {
    if (data.empty()) {
        return; // ignore empty lines
    }
    int sent = 0;
    int curSent = 0;

    while (sent < data.size()) {
        if ((curSent = send(sfd, data.substr(sent).data(), data.size() - sent, 0)) == -1) {
            perror("Response was not sent");
            throw ClientException("Request sending failed");
        }
        sent += curSent;
    }
}


void Client::epoll_ctl_add(int efd, int fd, uint32_t ev) {
    epollEvent.events = ev;
    epollEvent.data.fd = fd;

    if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &epollEvent) == -1) {
        throw ClientException("Epoll_ctl failed");
    }
}