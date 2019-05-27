#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include "utils.h"
#include <sys/epoll.h>
#include <array>
#include <cstring>

const size_t BUFFER_SIZE = 65536;
const size_t MAX_EVENTS = 50;

void print_help() {
    std::cout << "Usage: ./server PORT" << std::endl;
}

void process_connection(descriptor_wrapper const& descriptor, descriptor_wrapper const& epoll_descriptor) {
    struct epoll_event event;
    struct sockaddr client_addr;
    socklen_t len = sizeof(client_addr);
    int client_descriptor = accept(descriptor, (sockaddr *)&client_addr, &len);
    if (client_descriptor == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        print_error("accept failed: ");
        return;
    }
    if (!set_nonblocking(client_descriptor)) {
        return;
    }
    event.data.fd = client_descriptor;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, client_descriptor, &event) == -1) {
        print_error("epoll_ctl failed: ");
    }
    std::cout << "Connected with descriptor " << client_descriptor << std::endl;
}

bool process_message(int client_descriptor) {
    char buffer[BUFFER_SIZE];
    auto request_len = recv(client_descriptor, buffer, BUFFER_SIZE - 1, 0);
    if (request_len < 0) {
        print_error("recv failed: ");
        return false;
    }
    if (request_len == 0) {
        if (close(client_descriptor) == -1) {
            print_error("descriptor close failed: ");
        }
        std::cout << "Connection with descriptor " << client_descriptor << " was closed" << std::endl;
        return false;
    }
    buffer[request_len] = 0;
    std::cout << "received: " << buffer << std::endl;
    auto response_len = send(client_descriptor, buffer, request_len, 0);
    if (response_len == -1) {
        print_error("send failed: ");
        return false;
    }
    if (std::strcmp("stop", buffer) == 0) {
        std::cout << "Server stopped" << std::endl;
        return true;
    }
    return false;
}


int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Incorrect number of arguments\n";
        print_help();
        return EXIT_FAILURE;
    }
    if (!check_port(argv[1])) {
        std::cout << "Port number is incorrect" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Starting server on port " << argv[1] << std::endl;
    int status;
    struct addrinfo hints{};
    struct addrinfo *result, *chosen = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((status = getaddrinfo(nullptr, argv[1], &hints, &result)) != 0) {
        std::cerr << "getaddrinfo failed: " << gai_strerror(status);
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    descriptor_wrapper descriptor = -1;
    for (chosen = result; chosen != nullptr; chosen = chosen->ai_next) {
        descriptor = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (descriptor == -1) {
            print_error("Can't create socket, trying next: ");
            continue;
        }
        if (::bind(descriptor, result->ai_addr, result->ai_addrlen) == -1) {
            print_error("Can't bind socket, trying next: ");
            continue;
        }
        break;
    }
    if (chosen == nullptr) {
        std::cerr << "Socket wasn't created/binded, exiting" << std::endl;
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    if (!set_nonblocking(descriptor)) {
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    if (listen(descriptor, SOMAXCONN) == -1) {
        print_error("listen failed: ");
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }

    struct epoll_event event;
    descriptor_wrapper epoll_descriptor = epoll_create1(0);
    if (epoll_descriptor == -1) {
        print_error("epoll_create1 failed : ");
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    event.data.fd = descriptor;
    event.events = EPOLLIN | EPOLLET;
    std::array<struct epoll_event, MAX_EVENTS> events;
    if (!add_epoll(epoll_descriptor, descriptor, event)) {
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    bool alive = true;
    while (alive) {
        int num = epoll_wait(epoll_descriptor, events.data(), MAX_EVENTS, -1);
        if (num < 0) {
            print_error("epoll_wait failed: ");
            break;
        }
        for (size_t i = 0; i < num && alive; i++) {
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                print_error("epoll error: ");
                if (close(events[i].data.fd) == -1) {
                    print_error("close failed: ");
                }
            } else if (events[i].data.fd == descriptor) {
                process_connection(descriptor, epoll_descriptor);
            } else {
                if (process_message(events[i].data.fd)) {
                    alive = false;
                }
            }
        }
    }
    freeaddrinfo(result);
}
