//
// Created by SP4RK on 18/05/2019.
//
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <netinet/in.h>
#include <cstring>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/epoll.h>

static const int MAX_EVENTS = 4;
static const int BUFFER_SIZE = 1024;
static const int BACKLOG = 5;

int bind(int port) {
    int listenerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenerSocket < 0) {
        std::cerr << "Error while opening socket " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    int option = 1;
    int status;
    status = setsockopt(listenerSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option));
    if (status == -1) {
        std::cerr << "Error while setting option to socket " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    status = fcntl(listenerSocket, F_SETFL, fcntl(listenerSocket, F_GETFD, 0) | O_NONBLOCK);
    if (status == -1) {
        std::cerr << "Error while setting socket to non blocking mode: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    sockaddr_in address{};
    address.sin_family = AF_INET,
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;
    status = bind(listenerSocket, (struct sockaddr*) &address, sizeof(address));
    if (status == -1) {
        std::cerr << "Error while binding to socket " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    return listenerSocket;
}

void epollCtl(epoll_event &event, int events, int descriptor, int epollDescriptor) {
    event.events = events;
    event.data.fd = descriptor;
    int status = epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, descriptor, &event);
    if (status == -1) {
        std::cerr << "Error while performing epoll operation: " << strerror(errno) << std::endl;
        close(descriptor);
        close(epollDescriptor);
        exit(EXIT_FAILURE);
    }
}

void disconnect(int epollDescriptor, int descriptor) {
    if (epoll_ctl(epollDescriptor, EPOLL_CTL_DEL, descriptor, NULL) == -1) {
        std::cerr << "Error while disconnecting: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "You are expected to enter 1 arguments: <port>" << std::endl;
        exit(EXIT_FAILURE);
    }
    int epollDescriptor = epoll_create1(0);
    int listenerSocket = bind(atoi(argv[1]));
    std::cout << "Write 'stop' when you want server to stop" << std::endl;
    struct epoll_event epollEvent{};
    struct epoll_event changingEvents[MAX_EVENTS];

    int status = listen(listenerSocket, BACKLOG);
    if (status == -1) {
        std::cerr << "Error while initializing listening: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    epollCtl(epollEvent, EPOLLIN, listenerSocket, epollDescriptor);
    epollCtl(epollEvent, EPOLLIN, 0, epollDescriptor);
    bool running = true;
    while (running) {
        const int descriptorsNumber = epoll_wait(epollDescriptor, changingEvents, MAX_EVENTS, -1);
        if (descriptorsNumber == -1) {
            std::cerr << "Error while epoll waiting: " << strerror(errno) << std::endl;
        }
        for (int i = 0; i < descriptorsNumber; i++) {
            int descriptor = changingEvents[i].data.fd;
            if (descriptor == listenerSocket) {
                struct sockaddr_in addr{};
                socklen_t addrLen = sizeof(addr);
                int acceptingDescriptor = accept(listenerSocket, (struct sockaddr*) &addr, &addrLen);
                if (acceptingDescriptor == -1) {
                    std::cerr << "Error while connecting: " << strerror(errno) << std::endl;
                    continue;
                }
                std::cout << "Connected to client" << std::endl;
                const int flags = fcntl(acceptingDescriptor, F_GETFL, 0);
                fcntl(acceptingDescriptor, F_SETFL, flags | O_NONBLOCK);
                epollCtl(epollEvent, EPOLLIN | EPOLLET, acceptingDescriptor, epollDescriptor);
            } else if (descriptor == 0) {
                std::string line;
                std::getline(std::cin, line);
                if (line == "stop") {
                    running = false;
                }
            } else {
                char buffer[BUFFER_SIZE];
                while (ssize_t dataReceived = recv(connectionSocket, buffer, BUFFER_SIZE, 0)) {
                    if (dataReceived < 0) {
                        std::cerr << "Error while receiving message" << strerror(errno) << std::endl;
                        closeSocket(connectionSocket);
                        exit(EXIT_FAILURE);
                    } else if (dataReceived == 0) {
                        break;
                    } else {
                        ssize_t dataSent = 0;
                        while (dataSent < dataReceived) {
                            ssize_t curSent = send(connectionSocket, buffer + dataSent, dataReceived - dataSent, 0);
                            if (curSent < 0) {
                                std::cerr << "Error while sending message" << strerror(errno) << std::endl;
                                closeSocket(connectionSocket);
                                exit(EXIT_FAILURE);
                            } else {
                                dataSent += curSent;
                            }
                        }
                    }
                }
            }
        }
    }
}
