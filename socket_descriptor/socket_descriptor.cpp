//
// Created by Anton Shelepov on 2019-05-17.
//

#include "socket_descriptor.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

socket_descriptor::socket_descriptor() {
    descriptor = socket(AF_INET, SOCK_STREAM, 0);
}

socket_descriptor::socket_descriptor(int descriptor) : descriptor(descriptor) {}

socket_descriptor::~socket_descriptor() {
    if (descriptor != -1) {
        close(descriptor);
    }
}

socket_descriptor::socket_descriptor(socket_descriptor&& other) noexcept : descriptor(*other) {
    other.descriptor = -1;
}

socket_descriptor& socket_descriptor::operator=(socket_descriptor&& other) noexcept {
    descriptor = other.descriptor;
    other.descriptor = -1;

    return *this;
}

int socket_descriptor::operator*() const {
    return descriptor;
}

bool socket_descriptor::valid() const {
    return descriptor != -1;
}

socket_descriptor socket_descriptor::accept() {
    sockaddr_in client_addr = {0};

    socklen_t client_addr_len = sizeof(client_addr);
    return ::accept(descriptor, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
}
