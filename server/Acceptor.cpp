#include <fcntl.h>
#include <errno.h>

#include <unistd.h>
#include <string.h>
#include <algorithm>

#include "Server.h"
#include "Acceptor.h"

Acceptor::Acceptor(std::shared_ptr<Multiplexor> mult, int fd) : mult(mult), fd(fd) {
    mult->add_fd(fd, Multiplexor::OP_READ | Multiplexor::OP_EXCEPT, this);

    int flags = fcntl(fd, F_GETFL, 0);

    if(flags < 0) {
        throw ServerException(strerror(errno));
    }

    int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    if(ret < 0) {
        throw ServerException(strerror(errno));
    }

    handler = [this](){read();};

    printf("Acceptor created\n");
}

void Acceptor::self_destroy() {
    printf("Acceptor self-destroy\n");
    close();
    delete this;
}

void Acceptor::close() {
    ::close(fd);
}

void Acceptor::check_err() {
    int err;
    socklen_t len = sizeof(err);

    int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

    if (ret < 0) {
        fprintf(stderr, "Error getting SO_ERROR: %s\n", strerror(errno));
    }

    if (err < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
    }
}

void Acceptor::write() {
    check_err();

    int nwrtn = ::write(fd, message.c_str() + message_pos, message.size() - message_pos);

    if (nwrtn < 0 && errno != EINPROGRESS) {
        fprintf(stderr, "Error in write: %s\n", strerror(errno));
    }

    if (nwrtn > 0) {
        message_pos += nwrtn;
    }

    if (message_pos == message.size()) {
        handler = [](){ };
        mult->remove_fd(fd);
        self_destroy();
    }
}



void Acceptor::read() {
    check_err();

    int nread = ::read(fd, buf, sizeof(buf));

    if (nread < 0 && errno != EINPROGRESS) {
        fprintf(stderr, "Error in read: %s\n", strerror(errno));
    }

    if (nread > 0) {
        std::copy(buf, buf + nread, std::inserter(message, message.end()));
    }

    if (std::find(buf, buf + nread, '\n') != buf + nread) {
        handler = [this](){ write(); };
        mult->remove_fd(fd);
        mult->add_fd(fd, Multiplexor::OP_WRITE | Multiplexor::OP_EXCEPT, this);
    }
}
