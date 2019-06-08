#include "server.hpp"

#include <arpa/inet.h>
#include <inttypes.h>
#include <limits>
#include <sys/epoll.h>
#include <fcntl.h>

// @formatter:off
std::string const USAGE = "Simple ECHO server\n"
                          "Usage: ./server [address [port]]\n"
                          "\t- " + logger()._HELP + "address" + logger()._DEFAULT + " is 127.0.0.1\n"
                          "\t- " + logger()._HELP + "port" + logger()._DEFAULT + " is 8007";
// @formatter:on

unsigned int const server::MAX_QUEUE = 8;
unsigned int const server::BUFFER_SIZE = 2048;

server::server(std::string const &address, uint16_t port) : socket_desc(), server_address{0} {
    if (!socket_desc.check_valid()) {
        logger().fail("Could not create a fd for socket", errno);
        return;
    }
    memset(&server_address, 0, sizeof(sockaddr_in));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(address.c_str());

    if (bind(socket_desc.get_fd(), reinterpret_cast<sockaddr *>(&server_address), sizeof(sockaddr_in)) < 0) {
        logger().fail("Failed to run server with socket connection", errno);
        return;
    }
    if (listen(socket_desc.get_fd(), MAX_QUEUE) < 0) {
        logger().fail("Failed to set up a listening mode for connection", errno);
        return;
    }
    logger().success("Server bind and set up completed");
}

server::~server() {
    server_address = sockaddr_in{0};
    socket_desc.close();
}

void server::await_and_respond() {
    sockaddr_in client_address{0};
    socklen_t socket_len = sizeof(sockaddr_in);

    unsigned int EPOLL_MAX_EVENTS = 10;
    epoll_event epoll{}, events[EPOLL_MAX_EVENTS];
    fd_wrapper epoll_desc(epoll_create1(0));
    if (!epoll_desc.check_valid()) {
        logger().fail("Failed to create epoll fd", errno);
        return;
    }

    epoll.events = EPOLLIN;
    epoll.data.fd = socket_desc.get_fd();
    if (epoll_ctl(epoll_desc.get_fd(), EPOLL_CTL_ADD, socket_desc.get_fd(), &epoll) < 0) {
        logger().fail("Unable to add socket descriptor to interest list", errno);
        return;
    }

    while (true) {
        int count = epoll_wait(epoll_desc.get_fd(), events, EPOLL_MAX_EVENTS, -1);
        if (count < 0) {
            logger().fail("Failed to wait for epoll", errno);
            return;
        }

        for (unsigned int i = 0; i < count; i++) {
            if (events[i].data.fd == socket_desc.get_fd()) {
                memset(&client_address, 0, sizeof(sockaddr_in));
                int client = accept(socket_desc.get_fd(), reinterpret_cast<sockaddr *>(&client_address), &socket_len);
                if (client < 0) {
                    logger().fail("Could not accept a connection", errno);
                    continue;
                }

                if (fcntl(client, F_SETFL, fcntl(client, F_GETFL, 0) | O_NONBLOCK) < 0) {
                    logger().fail("Can not set up a non-blocking file descriptor", errno);
                    continue;
                }
                epoll.events = EPOLLIN | EPOLLOUT;
                epoll.data.fd = client;

                if (epoll_ctl(epoll_desc.get_fd(), EPOLL_CTL_ADD, client, &epoll) < 0) {
                    logger().fail("Can not add new client to epoll interest list", errno);
                    fd_wrapper tmp(client);
                    continue;
                }
            } else if ((events[i].events & EPOLLIN > 0) && (events[i].events &EPOLLOUT) > 0) {
                fd_wrapper client_desc(events[i].data.fd);

                char *buf = reinterpret_cast<char *>(malloc(BUFFER_SIZE));
                size_t buffer_size = 0;
                client_desc.simple_receive_message(buf, buffer_size, BUFFER_SIZE);

                char response[] = {' ', '-', 'd', 'e', ' ', 'g', 'o', 'z', 'a', 'r', 'u', '!', '\n'};
                for (int j = 0; buffer_size < BUFFER_SIZE && j < sizeof(response); buffer_size++, j++) {
                    buf[buffer_size] = response[j];
                }
                buf[buffer_size - 1] = '\n';
                client_desc.simple_send_message(buf, buffer_size);

                epoll.data.fd = client_desc.get_fd();
                if (epoll_ctl(epoll_desc.get_fd(), EPOLL_CTL_DEL, client_desc.get_fd(), &epoll) < 0) {
                    logger().fail("Could not remove an instance from epoll events", errno);
                    return;
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {

    if (argc > 3) {
        logger().fail("Invalid amount of arguments given");
        std::cout << USAGE << std::endl;
        return 0;
    }

    std::string address = "127.0.0.1";
    uint16_t port = 8007;

    if (argc > 1) {
        address = argv[1];
    }
    if (argc > 2) {
        char *endptr;
        intmax_t _port = strtoimax(argv[2], &endptr, 10);
        if (errno == ERANGE || _port < 0 || _port > std::numeric_limits<uint16_t>::max()
            || endptr == argv[1] || *endptr != '\0') {
            return logger().fail("Invalid port number");
        }
        port = static_cast<uint16_t>(_port);
    }

    server server(address, port);
    server.await_and_respond();

}