#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <iostream>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "file_descriptor.h"
#include "utils.h"

const size_t EPOLL_SIZE = 10;
const size_t BUFFER_SIZE = 100;

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
        error("Bad port, can't convert to int");
    }

    file_descriptor listener;
    listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (listener == -1) {
        error("Can't create socket");
    }

    struct sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = port;

    if (inet_pton(AF_INET, argv[1], &server.sin_addr) != 1) {
        error("Bad adress");
    }

    if (bind(listener, (sockaddr *) &server, sizeof(server)) == -1) {
        error("Can't bind");
    }

    if (listen(listener, 10) == -1) {
        error("Can't listen");
    }

    file_descriptor epolld;
    epolld = epoll_create(1);

    if (epolld == -1) {
        error("Can't create epoll");
    }

    epoll_event listener_event{}, events[EPOLL_SIZE];
    listener_event.data.fd = listener;
    listener_event.events = EPOLLIN;

    if (epoll_ctl(epolld, EPOLL_CTL_ADD, listener, &listener_event) < 0) {
        error("Can't add descriptor to epoll");
    }

    std::string hello = "hello ";
    char buffer[BUFFER_SIZE];

    bool working = true;
    while (working) {
        int ready = epoll_wait(epolld, events, EPOLL_SIZE, -1);

        if (ready == -1) {
            error("Can't wait epoll");
        }

        for (int i = 0; i < ready; i++) {
            int currFd = events[i].data.fd;
            if (currFd == listener) {

                struct sockaddr_in client;
                socklen_t addrlen = sizeof(client);

                int client_fd = accept(listener, (sockaddr *) &client, &addrlen);
                if (client_fd == -1) {
                    perror("Can't accept");
                    continue;
                }

                std::cout << "Connection with new client established!\n";

                fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);
                epoll_event client_event{};
                client_event.data.fd = client_fd;
                client_event.events = EPOLLIN;

                if (epoll_ctl(epolld, EPOLL_CTL_ADD, client_fd, &client_event) < 0) {
                    perror("Can't add descriptor to epoll");
                    close(client_fd);
                }
            } else {
                bzero(buffer, BUFFER_SIZE);
                ssize_t len = receive_message(currFd, buffer, BUFFER_SIZE);//recv(currFd, buffer, BUFFER_SIZE, 0);
                if (len == -1) {
                    perror("Can't receive a message");
                    continue;
                }

                if (len == 0) {
                    continue;
                }

                std::string message(buffer, buffer + len);
                if (message == "stop") {
                    working = false;
                }

                message = hello + message;

                send_message(message, currFd);
            }
        }
    }
}