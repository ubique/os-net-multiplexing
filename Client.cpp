#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include "Client.h"

Client::Client(char *address, uint16_t port) {
    openSocket();
    setUp(address, port);

}

void Client::openSocket() {
    mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mySocket < 0) {
        throw std::runtime_error("cannot open socket: " + std::string(strerror(errno)));
    }
}


void Client::setUp(char *address, uint16_t port) {
    myEpoll = epoll_create1(0);
    if (myEpoll == -1) {
        throw std::runtime_error("cannot create epoll");
    }
    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = 0;
    if (epoll_ctl(myEpoll, EPOLL_CTL_ADD, 0, &event) == -1) {
        throw std::runtime_error("Cannot add input stream to epoll");
    }
    if (fcntl(mySocket, F_SETFL, fcntl(mySocket, F_GETFD, 0) | O_NONBLOCK) < 0) {
        throw std::runtime_error("cannot work non-blocking");
    }
    sockaddr_in sockaddr{};
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = port;
    sockaddr.sin_addr.s_addr = inet_addr(address);
    if (connect(mySocket, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0) {
        if (errno != EINPROGRESS) {
            throw std::runtime_error("cannot connect to socket");
        }
    }
    event.events = EPOLLIN;
    event.data.fd = mySocket;
    if (epoll_ctl(myEpoll, EPOLL_CTL_ADD, mySocket, &event) == -1) {
        throw std::runtime_error("Cannot add to epoll");
    }


}

void Client::run() {
    bool isRunning = true;
    std::string message = "You are connected "; // first expected message from server
    while (isRunning) {
        int ready = epoll_wait(myEpoll, events, 6, -1);
        if (ready == -1) {
            throw std::runtime_error("error while waiting epoll");
        }
        for (int i = 0; i < ready; ++i) {
            if (events[i].data.fd == mySocket) {
                char answer[BUFFER_SIZE];
                ssize_t reqSize = 0;
                while (reqSize != message.length()) {
                    ssize_t cur = recv(mySocket, answer, BUFFER_SIZE, 0);
                    if (cur == 0) {
                        break;
                    }
                    if (cur < 0) {
                        throw std::runtime_error(
                                "error while getting answer from server: " + std::string(strerror(errno)));
                    }
                    reqSize += cur;
                }
                answer[reqSize] = '\0';
                std::cout << "answer from server:" << std::endl;
                std::cout << answer << std::endl;
                std::cout << "your request: " << std::endl;
            } else if (events[i].data.fd == 0) {
                std::getline(std::cin, message);
                if (message == "ex" || std::cin.eof()) {
                    isRunning = false;
                    continue;
                }
                ssize_t sentSize = 0;
                while (sentSize != message.size()) {
                    ssize_t cur = send(mySocket, message.data() + sentSize,
                                       static_cast<size_t>(message.length() - sentSize), 0);
                    if (cur == 0) {
                        break;
                    }
                    if (cur < 0) {
                        throw std::runtime_error(
                                "error while getting answer from server: " + std::string(strerror(errno)));
                    }
                    sentSize += cur;
                }

            }
        }
    }

}

Client::~Client() {
    close(mySocket);
    close(myEpoll);
    delete[] events;
}

