#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include "utils.h"
#include <cstring>

const size_t BUFFER_SIZE = 65536;

void print_help() {
    std::cout << "Usage: ./server PORT" << std::endl;
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
    int descriptor = -1;
    for (chosen = result; chosen != nullptr; chosen = chosen->ai_next) {
        descriptor = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (descriptor == -1) {
            print_error("Can't create socket, trying next: ");
            continue;
        }
        if (::bind(descriptor, result->ai_addr, result->ai_addrlen) == -1) {
            print_error("Can't bind socket, trying next: ");
            close_socket(descriptor);
            continue;
        }
        break;
    }
    if (chosen == nullptr) {
        std::cerr << "Socket wasn't created/binded, exiting" << std::endl;
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
  /*  if (!make_nonblocking_socket(descriptor)) {
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }*/
    if (listen(descriptor, SOMAXCONN) == -1) {
        print_error("listen failed: ");
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    while (true) {
        int client_descriptor = accept(descriptor, chosen->ai_addr, &chosen->ai_addrlen);
        if (client_descriptor == -1) {
            print_error("accept failed: ");
            continue;
        }
        auto request_len = recv(client_descriptor, buffer, BUFFER_SIZE - 1, 0);
       // auto request_len = recvfrom(descriptor, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_address, &len);
        if (request_len < 0) {
            print_error("recv failed: ");
            continue;
        }
        buffer[request_len] = 0;
        std::cout << "received: " << buffer << std::endl;
        auto response_len = send(client_descriptor, buffer, request_len, 0);
       // auto response_len = sendto(descriptor, buffer, request_len, 0, (struct sockaddr *)&client_address, len);
        if (response_len == -1) {
            print_error("send failed: ");
            continue;
        }
        if (std::strcmp("stop", buffer) == 0) {
            std::cout << "Server stopped" << std::endl;
            break;
        }
    }
    close_socket(descriptor);
    freeaddrinfo(result);
}
