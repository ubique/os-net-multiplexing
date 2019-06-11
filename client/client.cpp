//
// Created by Anton Shelepov on 2019-05-17.
//

#include "client.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "../utils/utils.h"
#include <fcntl.h>

client_exception::client_exception(std::string const& msg) : std::runtime_error(msg + ": " + std::string(strerror(errno))) {}

client::client() : socket_fd() {
    if (!socket_fd.valid()) {
        throw client_exception("Couldn't create socket");
    }

    epoll_fd = epoll_create(1);
    if (!epoll_fd.valid()) {
        throw client_exception("Couldn't create epoll");
    }

    add_to_epoll(STDIN_FILENO, EPOLLIN);
    log("Client was deployed successfully");
}

void client::establish_connection(std::string const& address, int port) {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(address.data());

    if (fcntl(*socket_fd, F_SETFL, O_NONBLOCK) == -1) {
        log("Couldn't change socket mode");
    }

    if (connect(*socket_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(sockaddr_in)) == -1) {
        throw client_exception("Couldn't connect to the server " + address);
    }

    add_to_epoll(*socket_fd, EPOLLIN);

    log("Connected to server " + address + ", port " + std::to_string(port));
}


void client::log(std::string const& msg) {
    std::cerr << "client: " << msg << std::endl;
}

void client::run() {
    alive = true;
    while (alive) {
        int ready = epoll_wait(*epoll_fd, events, MAX_EVENTS, -1);
        if (ready == -1) {
            throw client_exception("Epoll failed");
        }

        for (int i = 0; i < ready; i++) {
            uint32_t mode = events[i].events;
            int desc = events[i].data.fd;

            try {
                if(desc == *socket_fd) {
                    std::cout << read(*socket_fd) << std::endl;
                } else {
                    if(desc == STDIN_FILENO) {
                        std::string request;

                        getline(std::cin, request);

                        if(!std::cin || request == "exit") {
                            alive = false;
                            break;
                        }

                        if (!request.empty()) {
                            send(*socket_fd, request);
                        }
                    }
                }
            } catch (client_exception const& e) {
                log(e.what());
            }
        }
    }
}

void client::add_to_epoll(int sd, uint32_t events) {
    epoll_event event;
    event.data.fd = sd;
    event.events = events;

    if (epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, sd, &event) == -1) {
        throw client_exception("Couldn't add descriptor to epoll");
    }
    log("Descriptor " + std::to_string(sd) + " was added to epoll");
}

std::string client::read(int desc) {
    std::string message = utils::read(desc);

    if (message.size() == 0) {
        alive = false;
        throw client_exception("Couldn't read respond from socket " + std::to_string(desc));
    }

    return message;
}

void client::send(int desc, std::string const& message) {
    size_t was_send = utils::send(desc, message);

    if (was_send == 0) {
        alive = false;
        throw client_exception("Couldn't send request to socket " + std::to_string(desc));
    }
    if (was_send != message.size()) {
        log("Not full message was sent");
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Simple echo client. Server will echo your requests. Type \"exit\" request to exit" << std::endl;
    std::string address = "127.0.0.1";
    int port = 90190;
    if (argc < 3) {
        std::cout << "Expected address and port number; default values will be used: 127.0.0.1 90190" << std::endl;
    } else {
        address = argv[1];
        port = std::stoi(argv[2]);
    }

    try {
        client cli;
        cli.establish_connection(address, port);
        cli.run();
    } catch (client_exception& e) {
        std::cout << e.what() << std::endl;
    }
}
