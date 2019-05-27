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
#include <sys/epoll.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>


static const int MAX_EVENTS = 4;
static const int MAX_BUFFER_SIZE = 4096;

int connect(int port) {
    int openedSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (openedSocket < 0) {
        std::cerr << "Error while opening socket " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    int option = 1;
    int status;
    status = setsockopt(openedSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (status == -1) {
        std::cerr << "Error while setting option to socket " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    status = connect(openedSocket, (struct sockaddr*) &address, sizeof(address));
    if (status == -1) {
        std::cerr << "Error while connecting to socket " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    return openedSocket;
}

void epollCtl(epoll_event &event, int events, int descriptor, int epollDescriptor) {
    event.events = events;
    event.data.fd = descriptor;
    int status = epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, descriptor, &event);
    if (status == -1) {
        std::cerr << "Error while connecting epoll with socket: " << strerror(errno) << std::endl;
        close(descriptor);
        close(epollDescriptor);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "You are expected to enter 1 arguments: <port>" << std::endl;
        exit(EXIT_FAILURE);
    }
    int epollDescriptor = epoll_create1(0);
    int connectedSocket = connect(atoi(argv[1]));
    std::cout << "You can enter any message and it will be sent to server and, hopefully, will return as a reply \n"
                 "Write 'exit' when you want to stop" << std::endl;
    struct epoll_event epollEvent{};
    struct epoll_event changingEvents[MAX_EVENTS];
    epollCtl(epollEvent, EPOLLIN, connectedSocket, epollDescriptor);
    epollCtl(epollEvent, EPOLLIN, 0, epollDescriptor);
    bool running = true;
    while (running) {
        int descriptorsNumber = epoll_wait(epollDescriptor, changingEvents, MAX_EVENTS, -1);
        if (descriptorsNumber == -1) {
            std::cerr << "Error while waiting: " << std::endl;
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < descriptorsNumber; i++) {
            if (changingEvents[i].data.fd == connectedSocket) { //from server
                char buffer[MAX_BUFFER_SIZE];
                int receivedLength = recv(connectedSocket, buffer, MAX_BUFFER_SIZE, 0);
                if (receivedLength == -1) {
                    std::cerr << "Response wasn't received: " << std::endl;
                    exit(EXIT_FAILURE);
                } else if (receivedLength == 0) {
                    std::cout << "Lost connection to server" << std::endl;
                    running = false;
                } else {
                    std::cout << "Reply from server: " << buffer << std::endl;
                }
            } else if (changingEvents[i].data.fd == 0) { //from standard input
                std::string line;
                std::getline(std::cin, line);
                if (line == "exit") {
                    close(connectedSocket);
                    exit(EXIT_SUCCESS);
                } else {
                    if (send(connectedSocket, line.data(), line.size(), 0) == -1) {
                        std::cerr << "Error while sending request: " << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
}