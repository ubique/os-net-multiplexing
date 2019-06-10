//
// Created by Anton Shelepov on 2019-05-17.
//

#include "server.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <vector>
#include <sys/types.h>
#include "../socket_descriptor/socket_descriptor.h"
#include <sys/epoll.h>
#include <cstring>
#include "../utils/utils.h"

server_exception::server_exception(std::string const& msg) : std::runtime_error(msg + ": " + strerror(errno)) {}

void server::log(const std::string& msg) {
    std::cerr << "server: " + msg << std::endl;
}

server::server(std::string const& address, int port) : socket_fd() {

    if (!socket_fd.valid()) {
        throw server_exception("Couldn't create socket");
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(address.data());
    addr.sin_port = htons(port);

    epoll_fd = epoll_create(1);

    if (!epoll_fd.valid()) {
        throw server_exception("Couldn't create epoll");
    }

    add_to_epoll(*socket_fd, EPOLLIN);

    if (bind(*socket_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        throw server_exception("Couldn't bind server address to the socket");
    }

    if (listen(*socket_fd, BACKLOG_QUEUE_SIZE) == -1) {
        throw server_exception("Couldn't start listening to the socket");
    }

    log("Server was deployed successfully");
}

void server::run() {
    log("Waiting for connections");

    while (true) {
        int ready = epoll_wait(*epoll_fd, events, MAX_EVENTS, 500);

        if (ready == -1) {
            log("Epoll wait failed");
            continue;
        }

        for (int i = 0; i < ready; i++) {
            uint32_t mode = events[i].events;
            int desc = events[i].data.fd;

            try {
                if(mode & EPOLLIN) {
                    if(desc == *socket_fd) {
                        socket_descriptor client = socket_fd.accept();

                        if(client.valid()) {
                            add_to_epoll(*client, EPOLLIN);
                            sockets[*client] = std::move(client);
                            log("Client added");
                        }

                    } else {
                        send(desc, read(desc));
                    }
                }
            } catch (server_exception const& e) {
                log(e.what());
            }
        }
    }
}

void server::add_to_epoll(int sd, uint32_t events) {
    epoll_event event;
    event.data.fd = sd;
    event.events = events;

    if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, sd, &event) == -1) {
        throw server_exception("Couldn't add descriptor to epoll");
    }

}

void server::remove_from_epoll(int sd) {
    if (epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, sd, nullptr) == -1) {
        throw server_exception("Couldn't remove descriptor from epoll");
    }

    sockets.erase(sd);
    log("Descriptor was removed from epoll " + std::to_string(sd));
}

std::string server::read(int desc) {
    std::string message = utils::read(desc);

    if (message.size() == 0) {
        remove_from_epoll(desc);
    }

    return message;
}

void server::send(int desc, std::string const& message) {
    size_t was_send = utils::send(desc, message);

    if (was_send == 0) {
        throw server_exception("Couldn't send respond to " + std::to_string(desc));
    }
}

int main(int argc, char* argv[]) {
    std::string address = "127.0.0.1";
    int port = 90190;
    if (argc < 3) {
        std::cout << "Expected address and port number; default values will be used: 127.0.0.1 90190" << std::endl;
    } else {
        address = argv[1];
        port = std::stoi(argv[2]);
    }

    try {
        server serv(address, port);
        serv.run();
    } catch (server_exception& e) {
        std::cout << e.what() << std::endl;
    }
}