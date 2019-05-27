#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <array>
#include "utils.h"

const size_t BUFFER_SIZE = 65536;
const size_t MAX_EVENTS = 50;

void print_help() {
    std::cout << "Usage: ./client IP PORT\n" << std::endl;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Wrong number of arguments\n";
        print_help();
        return EXIT_FAILURE;
    }
    if (!check_port(argv[2])) {
        std::cout << "Port number is incorrect" << std::endl;
        return EXIT_FAILURE;
    }
    int status;
    struct addrinfo hints{};
    struct addrinfo *result, *chosen;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((status = getaddrinfo(argv[1], argv[2], &hints, &result)) != 0) {
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
        break;
    }
    if (chosen == nullptr) {
        std::cerr << "Socket wasn't created, exiting" << std::endl;
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    char buffer[BUFFER_SIZE];
    if (!set_nonblocking(descriptor)) {
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    descriptor_wrapper epoll_descriptor = epoll_create1(0);
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = descriptor;
    if (!add_epoll(epoll_descriptor, descriptor, event)) {
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    event.data.fd = STDIN_FILENO;
    if (!add_epoll(epoll_descriptor, STDIN_FILENO, event)) {
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    if (connect(descriptor, chosen->ai_addr, chosen->ai_addrlen) == -1 && errno != EINPROGRESS) {
        print_error("connect failed: ");
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    std::array<struct epoll_event, MAX_EVENTS> events;
    std::string line;
    std::cout << "Type stop to stop server" << std::endl;
    bool alive = true;
    while (alive) {
        int num = epoll_wait(epoll_descriptor, events.data(), MAX_EVENTS, -1);
        if (num == -1) {
            print_error("epoll_wait failed: ");
            break;
        }
        for (size_t i = 0; i < num; i++) {
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                print_error("epoll error: ");
                if (close(events[i].data.fd) == -1) {
                    print_error("close failed: ");
                }
            } else if (events[i].data.fd == descriptor) {
                auto request_len = recv(descriptor, &buffer, BUFFER_SIZE - 1, 0);
                if (request_len == -1) {
                    print_error("recv failed: ");
                    freeaddrinfo(result);
                    continue;
                }
                if (request_len == 0) {
                    std::cout << "Connection closed" << std::endl;
                    alive = false;
                    break;
                }
                buffer[request_len] = 0;
                std::cout << "Response received: " << buffer << std::endl;
            } else {
                getline(std::cin, line);
                if (std::cin.eof()) {
                    alive = false;
                    break;
                }
                if (line == "") {
                    continue;
                }
                std::cout << "Sending \"" << line << "\" to server" << std::endl;
                auto response_len = send(descriptor, line.data(), line.size(), 0);
                if (response_len < 0) {
                    print_error("send failed: ");
                    continue;
                }
            }
        }
    }
    freeaddrinfo(result);

}
