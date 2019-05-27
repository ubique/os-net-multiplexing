#include "Server.h"
#include "Acceptor.h"

#include <algorithm>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

Server::Server(std::shared_ptr<Multiplexor> mult) : mult(std::move(mult)) {
    socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (socket == -1) {
        throw ServerException(strerror(errno));
    }
}

void Server::bind(sockaddr *addr, size_t len) {
    int ret = ::bind(socket, addr, len);

    if (ret < 0) {
        throw ServerException(strerror(errno));
    }

    int flags = fcntl(socket, F_GETFL, 0);

    if(flags < 0) {
        throw ServerException(strerror(errno));
    }

    int err = fcntl(socket, F_SETFL, flags | O_NONBLOCK);

    if(err < 0) {
        throw ServerException(strerror(errno));
    }

    if (::listen (socket, BACKLOG) < 0) {
        throw ServerException(strerror(errno));
    }

    mult->add_fd(socket, Multiplexor::OP_READ, this);

    handler = [this](){ accept(); };
}

void Server::check_err() {
    int err;
    socklen_t len = sizeof(err);

    int ret = getsockopt(socket, SOL_SOCKET, SO_ERROR, &err, &len);

    if (ret < 0) {
        fail = true;
        throw ServerException(strerror(errno));
    }

    if (err < 0) {
        fail = true;
        throw ServerException(strerror(errno));
    }
}

void Server::accept() {
    check_err();

    int fd = ::accept(socket, nullptr, nullptr);

    if (fd < 0) {
        std::cout << "WARN, accept() failed: " << strerror(errno) << std::endl;
    } else {
        Acceptor *acc = new Acceptor(mult, fd);  // It will delete(this) when needed
    }
}
