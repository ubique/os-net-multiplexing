#include "Client.h"

#include <algorithm>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

Client::Client(std::shared_ptr<Multiplexor> mult) : mult(mult) {
    socket = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

    if (socket == -1) {
        throw ClientException(strerror(errno));
    }
}

void Client::connect(sockaddr *addr, size_t len) {
    int ret = ::connect(socket, addr, len);

    if (ret == -1 && errno != EINPROGRESS) {
        fail = true;
        throw ClientException(strerror(errno));
    }

    mult->add_fd(socket, Multiplexor::OP_WRITE | Multiplexor::OP_EXCEPT, this);

    handler = [this](){ write(); };
}

void Client::close() {
    ::close(socket);
}

void Client::check_err() {
    int err;
    socklen_t len = sizeof(err);

    int ret = getsockopt(socket, SOL_SOCKET, SO_ERROR, &err, &len);

    if (ret < 0) {
        fail = true;
        throw ClientException(strerror(errno));
    }

    if (err < 0) {
        fail = true;
        throw ClientException(strerror(errno));
    }
}

void Client::write() {
    check_err();

    int nwrtn = ::write(socket, message.c_str() + message_pos, message.size() - message_pos);

    if (nwrtn < 0 && errno != EINPROGRESS) {
        fail = true;
        throw ClientException(strerror(errno));
    }

    if (nwrtn > 0) {
        message_pos += nwrtn;
    }

    if (message_pos == message.size()) {
        handler = [this](){ read(); };
        mult->remove_fd(socket);
        mult->add_fd(socket, Multiplexor::OP_READ | Multiplexor::OP_EXCEPT, this);

        std::cout << "Sent: " << message << std::endl;
    }
}



void Client::read() {
    check_err();

    int nread = ::read(socket, buf, sizeof(buf));

    if (nread < 0 && errno != EINPROGRESS) {
        fail = true;
        throw ClientException(strerror(errno));
    }

    if (nread > 0) {
        std::copy(buf, buf + nread, std::inserter(response, response.end()));
    }

    if (std::find(buf, buf + nread, '\n') != buf + nread) {
        handler = [](){};
        mult.get()->remove_fd(socket);
        close();
        ready = true;
        std::cout << "Got: " << response << std::endl;
    }
}
