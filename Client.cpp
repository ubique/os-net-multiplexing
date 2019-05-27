#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include "Client.h"

Client::Client(char *address, uint16_t port) {
    openSocket();
    makeConnection(address, port);
    createEpoll();


}

void Client::openSocket() {
    mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mySocket < 0) {
        throw std::runtime_error("cannot open socket: " + std::string(strerror(errno)));
    }
}

void Client::makeConnection(char *address, uint16_t port) {
    sockaddr_in sockaddr{};
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = port;
    sockaddr.sin_addr.s_addr = inet_addr(address);
    if (connect(mySocket, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0) {
        throw std::runtime_error("cannot connect to socket: " + std::string(strerror(errno)));
    }

}

void Client::createEpoll() {
    epoll_event event;
    myEpoll = epoll_create1(0);
    if (myEpoll == -1) {
        throw std::runtime_error("cannot create epoll");
    }
    event.events = EPOLLIN;
    event.data.fd = mySocket;
    if (epoll_ctl(myEpoll, EPOLL_CTL_ADD, mySocket, &event) == -1) {
        throw std::runtime_error("Cannot add to epoll");
    }
    event.events = EPOLLIN;
    event.data.fd = 0;

    if (epoll_ctl(myEpoll, EPOLL_CTL_ADD, 0, &event) == -1) {
        throw std::runtime_error("Cannot add input stream to epoll");
    }


}

void Client::run() {
    bool isRunning = true;
    std::string message;
    while (isRunning) {
        int ready = epoll_wait(myEpoll, events, 6, -1);
        if (ready == -1) {
            throw std::runtime_error("error while waiting epoll");
        }
        for (int i = 0; i < ready; ++i) {
            if (events[i].data.fd == mySocket) {
                char answer[BUFFER_SIZE];
                ssize_t reqByteNum = recv(mySocket, answer, BUFFER_SIZE, 0);
                if (reqByteNum == 0) {
                    std::cout << "server disconnected" << std::endl;
                    isRunning = false;
                    continue;
                }
                if (reqByteNum < 0) {
                    throw std::runtime_error("error while getting answer from server: " + std::string(strerror(errno)));
                }
                answer[reqByteNum] = '\0';
                std::cout << "answer from server:" << std::endl;
                std::cout << answer << std::endl;
                std::cout << "your request: " << std::endl;
            } else if (events[i].data.fd == 0) {
                std::getline(std::cin, message);
                if (message == "ex" || std::cin.eof()) {
                    isRunning = false;
                    continue;
                }
                if (send(mySocket, message.data(), message.length(), 0) == -1) {
                    throw std::runtime_error("error while sending to server: " + std::string(strerror(errno)));

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

