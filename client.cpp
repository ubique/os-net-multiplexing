#include <iostream>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/epoll.h>
#include "file_descriptor.h"
#include "utils.h"

size_t const MAX_EPOLL_EVENTS = 100;
size_t const BUFFER_SIZE = 1000;

void error(std::string const &message) {
    perror(message.data());
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "usage : ./server addr port";
        exit(EXIT_FAILURE);
    }

    int port;
    try {
        port = htons(std::stoi(argv[2]));
    } catch (std::invalid_argument &e) {
        std::cout << "Bad port, can't convert to int";
        exit(EXIT_FAILURE);
    }

    file_descriptor fd;
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (fd == -1) {
        perror("Can't create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = port;

    if (inet_pton(AF_INET, argv[1], &server.sin_addr) != 1) {
        perror("Bad adress");
        exit(EXIT_FAILURE);
    }

    if (connect(fd, (sockaddr *) (&server), sizeof(server)) < 0 && errno != EINPROGRESS) {
        perror("Can't connect.");
        exit(EXIT_FAILURE);
    }

    file_descriptor epolld;
    epolld = epoll_create(1);
    struct epoll_event writer_event, reader_event;
    struct epoll_event events[MAX_EPOLL_EVENTS];
    writer_event.events = EPOLLOUT;
    writer_event.data.fd = fd;

    reader_event.events = EPOLLIN;
    reader_event.data.fd = fd;
    if (epoll_ctl(epolld, EPOLL_CTL_ADD, fd, &writer_event) < 0) {
        error("Can't add descriptor to epoll");
    }

    std::string message;
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);

    bool working = true;
    while (working) {
        bzero(buffer, BUFFER_SIZE);
        std::cin >> message;

        if (message == "stop") {
            working = false;
        }

        if (epoll_ctl(epolld, EPOLL_CTL_MOD, fd, &writer_event) < 0) {
            error("Can't modify descriptor to write mode");
        }

        int ready = epoll_wait(epolld, events, MAX_EPOLL_EVENTS, -1);
        if (ready < 0) {
            error("Can't wait epoll");
        }

        for (int i = 0; i < ready; i++) {
            int currFd = events[i].data.fd;
            if (currFd == fd) {
                send_message(message, fd);
            }
        }

        if (epoll_ctl(epolld, EPOLL_CTL_MOD, fd, &reader_event) < 0) {
            error("Can't modify descriptor to read mode");
        }

        ready = epoll_wait(epolld, events, MAX_EPOLL_EVENTS, -1);
        if (ready < 0) {
            error("Waiting for input failed");
        }

        for (int i = 0; i < ready; i++) {
            int currFd = events[i].data.fd;

            if (currFd == fd) {
                ssize_t len = receive_message(fd, buffer, BUFFER_SIZE);
                if (len == -1) {
                    perror("Can't read response.");
                    continue;
                }

                std::cout << buffer << '\n';
                break;
            }
        }
    }
}