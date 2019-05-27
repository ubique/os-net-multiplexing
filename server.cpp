//
// Created by dumpling on 30.04.19.
//

#include<iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "fd_wrapper.h"

static const int LISTEN_BACKLOG = 50;
static const int BUF_SIZE = 4096;
static const int MAX_EVENTS = 128;

void print_err(const std::string &);

uint16_t get_port(const std::string &);

void send_all(int, const char *, int);

void print_help() {
    std::cout << "Usage: ./server [port] [address]" << std::endl;
}

fd_wrapper start_server(std::string &address, uint16_t port) {

    fd_wrapper sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        throw std::runtime_error("Can't create socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    if (bind(sfd.get(), reinterpret_cast<sockaddr *>(&addr), sizeof(sockaddr_in)) == -1) {
        throw std::runtime_error("Bind failed");
    }

    return sfd;
}

int echo(int reader) {
    char buf[BUF_SIZE];
    int bytes_read;

    bytes_read = recv(reader, buf, BUF_SIZE, 0);

    if (bytes_read == -1) {
        throw std::runtime_error("Recv failed");
    }

    if (bytes_read != 0) {
        send_all(reader, buf, bytes_read);
    }

    return bytes_read;
}

void wait_for_connections(const fd_wrapper &listener) {

    if (listen(listener.get(), LISTEN_BACKLOG) == -1) {
        throw std::runtime_error("Listen failed");
    }

    epoll_event ev{}, events[MAX_EVENTS];

    fd_wrapper epollfd = epoll_create1(0);
    if (epollfd == -1) {
        throw std::runtime_error("Epoll_create1 failed");
    }

    ev.events = EPOLLIN;
    ev.data.fd = listener.get();

    if (epoll_ctl(epollfd.get(), EPOLL_CTL_ADD, listener.get(), &ev) == -1) {
        throw std::runtime_error("Epoll_ctl failed");
    }


    while (true) {

        int cnt = epoll_wait(epollfd.get(), events, MAX_EVENTS, -1);

        if (cnt == -1) {
            throw std::runtime_error("Epoll_wait failed");
        }

        for (int i = 0; i < cnt; ++i) {
            if (events[i].data.fd == listener.get()) {
                int client = accept4(listener.get(), nullptr, nullptr, SOCK_NONBLOCK);

                if (client == -1) {
                    throw std::runtime_error("Accept failed");
                }

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client;
                if (epoll_ctl(epollfd.get(), EPOLL_CTL_ADD, client, &ev) == -1) {
                    throw std::runtime_error("Epoll_ctl add failed");
                }
            } else {
                int client = events[i].data.fd;
                if (!echo(client)) {
                    if (epoll_ctl(epollfd.get(), EPOLL_CTL_DEL, client, nullptr) == -1) {
                        throw std::runtime_error("Epoll_ctl del failed");
                    }

                    if (close(client) == -1) {
                        throw std::runtime_error("Close client socket failed");
                    }
                }
            }
        }
    }
}


int main(int argc, char **argv) {

    std::string address = "127.0.0.1";
    uint16_t port = 3456;

    if (argc > 1) {

        if (std::string(argv[1]) == "help") {
            print_help();
            return EXIT_SUCCESS;
        }

        try {
            port = get_port(std::string(argv[1]));
        } catch (std::invalid_argument &e) {
            print_err(e.what());
            return EXIT_FAILURE;
        } catch (std::out_of_range &e) {
            print_err("Port is too big");
            return EXIT_FAILURE;
        }
    }

    if (argc > 2) {
        address = std::string(argv[2]);
    }

    if (argc > 3) {
        print_err("Wrong arguments, use help");
        return EXIT_FAILURE;
    }


    try {
        fd_wrapper listener = start_server(address, port);
        wait_for_connections(listener);
    } catch (std::runtime_error &e) {
        print_err(e.what());
        return EXIT_FAILURE;
    }
}

