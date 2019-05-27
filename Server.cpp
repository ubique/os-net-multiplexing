
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <iostream>

#include "Server.h"

Server::Server(char *address, uint16_t port) {
    openSocket();
    setUp(address, port);
    createEpoll();
}

void Server::openSocket() {
    mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mySocket < 0) {
        throw std::runtime_error("cannot open socket: " + std::string(strerror(errno)));
    }
}

void Server::createEpoll() {
    myEpoll = epoll_create1(0);
    if (myEpoll == -1) {
        throw std::runtime_error("cannot create epoll");
    }
    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = mySocket;
    if (epoll_ctl(myEpoll, EPOLL_CTL_ADD, mySocket, &event) == -1) {
        throw std::runtime_error("Cannot register descriptor");
    }


}

void Server::setUp(char *address, uint16_t port) {
    sockaddr_in sockaddr{};
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = port;
    sockaddr.sin_addr.s_addr = inet_addr(address);
    if (bind(mySocket, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0) {
        throw std::runtime_error("cannot bind the socket: " + std::string(strerror(errno)));
    }

    if (listen(mySocket, 1) < 0) {
        throw std::runtime_error("cannot listen the socket: " + std::string(strerror(errno)));
    }

}

void Server::start() {

    while (true) {
        int ready = epoll_wait(myEpoll, events, 6, -1);
        if (ready == -1) {
            throw std::runtime_error("error while waiting epoll");
        }
        for (int i = 0; i < ready; ++i) {
            if (events[i].data.fd == mySocket) {
                int acceptSocket = accept(mySocket, nullptr, nullptr);
                if (acceptSocket < 0) {
                    std::cerr << "cannot accept connection to the socket" << std::endl;
                    continue;
                }
                if (fcntl(mySocket, F_SETFL, fcntl(mySocket, F_GETFD, 0) | O_NONBLOCK) < 0) {
                    std::cerr << "cannot work non-blocking" << std::endl;
                    continue;
                }
                epoll_event event;
                event.events = EPOLLIN;
                event.data.fd = acceptSocket;
                if (epoll_ctl(myEpoll, EPOLL_CTL_ADD, acceptSocket, &event) < 0) {
                    std::cerr << "cannot add to epoll" << std::endl;
                    continue;
                }
                send(acceptSocket, "You are connected", BUFFER_SIZE, 0);
            } else {
                char request[BUFFER_SIZE];
                int acceptSocket = events[i].data.fd;
                ssize_t reqBytesNum = recv(acceptSocket, request, BUFFER_SIZE, 0);
                if (reqBytesNum == 0) {
                    std::cout << "client " << acceptSocket << " disconnected" << std::endl;
                    if (epoll_ctl(myEpoll, EPOLL_CTL_DEL, acceptSocket,
                                  nullptr) == -1) {
                        std::cerr << "cannot close connection" << std::endl;
                    }
                    if (close(acceptSocket) < 0) {
                        std::cerr << "cannot close connection" << std::endl;
                    }
                    continue;
                } else if (reqBytesNum < 0) {
                    std::cerr << "cannot receive data from client" << std::endl;
                    continue;
                }
                request[reqBytesNum] = '\0';
                std::cout << "got from client " << acceptSocket << ": " << request << std::endl;
                if (send(acceptSocket, request, static_cast<size_t >(reqBytesNum), 0) < 0) {
                    std::cerr << "cannot send to client" << std::endl;
                } else {
                    std::cout << "data sent to client " << acceptSocket << std::endl;
                }
            }
        }

    }
}

Server::~Server() {
    close(mySocket);
    close(myEpoll);
    delete[] events;
}

