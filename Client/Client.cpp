//
// Created by Павел Пономарев on 2019-05-27.
//

#include "Client.h"
#include "ClientException.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <iostream>
#include <cstring>

Client::Client(int port) : mPort(port) {
    mSocket = openSocket();
    mEpoll = epoll_create(POLL_SIZE);
    if (mEpoll < 0) {
        throw ClientException(getMessage("Can't create epoll"));
    }
    if (!addToPoll(mEpoll, mSocket, EPOLLIN)) {
        throw ClientException(getMessage("Epoll_ctl failed"));
    }
    if (!addToPoll(mEpoll, 0, EPOLLIN)) {
        throw ClientException(getMessage("Epoll_ctl failed"));
    }
}


void Client::run() {
    struct sockaddr_in addr{};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(mPort);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(mSocket, (struct sockaddr*) &addr, sizeof(addr)) < 0 && errno != EINPROGRESS) {
        throw ClientException(getMessage("Connect failed"));
    }
    std::string str;
    while (true) {
        int ready = epoll_wait(mEpoll, events, POLL_SIZE, -1);
        if (ready < 0) {
            throw ClientException("Wait failed");
        }
        for (size_t i = 0; i < ready; ++i) {
             if (events[i].data.fd == mSocket) {
                 ssize_t length;
                 while (length = recv(mSocket, responseBuffer, BUFFER_SIZE, 0)) {
                     if (length == -1) {
                         if (errno == EAGAIN || errno == EWOULDBLOCK) {
                             break;
                         }
                         throw ClientException("Receive failure");
                     } else {
                         std::cout << std::string(responseBuffer, length) << std::endl;
                     }
                 }
             } else if (events[i].data.fd == 0) {
                 std::getline(std::cin, str);
                 if (str == "quit") {
                     return;
                 } else {
                     sendAll(str);
                 }
             }
        }
    }
}

int Client::openSocket() {
    int sock;
    sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sock < 0) {
        throw ClientException(getMessage("Failed to create socket"));
    }
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        throw ClientException(getMessage("Failed to set socket options"));
    }
    return sock;
}

bool Client::addToPoll(int efd, int fd, uint32_t ev) {
    event.events = ev;
    event.data.fd = fd;
    return (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event) >= 0);
}

std::string Client::getMessage(std::string const& message) {
    return std::string(message) + ": " + std::strerror(errno);
}

Client::~Client() {
    if (close(mSocket) < 0) {
        perror("Descriptor (socket) was not closed");
    }
    if (close(mEpoll) < 0) {
        perror("Descriptor (epoll) was not closed");
    }
}

void Client::sendAll(std::string const& str) {
    int sent = 0, cur = 0;
    while (sent < str.size()) {
        if ((cur = send(mSocket, str.substr(sent).data(), str.size() - sent, 0)) == -1) {
            throw ClientException("Request failure");
        }
        sent += cur;
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "incorrect number of arguments";
        return EXIT_FAILURE;
    }
    int port = atoi(argv[1]);
    try {
        Client client(port);
        client.run();
    } catch (ClientException& e) {
        std::cout << e.what();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}