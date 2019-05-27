#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include "utils.h"

const size_t BUFFER_SIZE = 65536;
const size_t MAX_EVENTS = 50;

void print_help() {
    std::cout << "Usage: ./client IP PORT" << std::endl;
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
    if (connect(descriptor, chosen->ai_addr, chosen->ai_addrlen) == -1) {
        print_error("connect failed: ");
        freeaddrinfo(result);
        return EXIT_FAILURE;
    }
    std::string line;
    while(true) {
        std::cout << "Type mesaage: " << std::flush;
        getline(std::cin, line);
        if (line == "") {
            continue;
        }
        if (std::cin.eof()) {
            break;
        }
        std::cout << "Sending \"" << line << "\" to " << argv[1] << ":" << argv[2] << std::endl;
        auto response_len = send(descriptor, line.data(), line.size(), 0);
        if (response_len < 0) {
            print_error("send failed: ");
            freeaddrinfo(result);
            break;
        }
        auto request_len = recv(descriptor, &buffer, BUFFER_SIZE - 1, 0);
        if (request_len == -1) {
            print_error("recv failed: ");
            freeaddrinfo(result);
            break;
        }
        buffer[request_len] = 0;
        std::cout << "Response received: " << buffer << std::endl;
    }
    freeaddrinfo(result);

}
