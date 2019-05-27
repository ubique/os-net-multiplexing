//
// Created by dumpling on 30.04.19.
//

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <bits/locale_facets.tcc>

#include "fd_wrapper.h"

static const int BUF_SIZE = 4096;
static const int MAX_EVENTS = 2;

void print_err(const std::string &);

uint16_t get_port(const std::string &);

void send_all(int, const char *, int);

void print_help() {
    std::cout << "Usage: ./client [port] [address]" << std::endl;
}

fd_wrapper open_connection(std::string &address, uint16_t port) {

    fd_wrapper sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        throw std::runtime_error("Can't create socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    if (connect(sfd.get(), reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
        throw std::runtime_error("Can't connect to the server");
    }

    return sfd;
}

void send_and_receive(const fd_wrapper &sender) {

    epoll_event ev{}, events[MAX_EVENTS];

    fd_wrapper epollfd = epoll_create1(0);
    if (epollfd == -1) {
        throw std::runtime_error("Epoll_create1 failed");
    }

    ev.events = EPOLLIN;
    ev.data.fd = sender.get();

    if (epoll_ctl(epollfd.get(), EPOLL_CTL_ADD, sender.get(), &ev) == -1) {
        throw std::runtime_error("Epoll_ctl failed");
    }

    ev.events = EPOLLIN;
    ev.data.fd = 0;

    if (epoll_ctl(epollfd.get(), EPOLL_CTL_ADD, 0, &ev) == -1) {
        throw std::runtime_error("Epoll_ctl failed");
    }


    char buf[BUF_SIZE];
    int bytes_read;
    int sum = 0;
    int expected_sum = 0;
    std::string message;

    bool process = true;
    while (process || sum < expected_sum) {
        int cnt = epoll_wait(epollfd.get(), events, MAX_EVENTS, -1);

        if (cnt == -1) {
            throw std::runtime_error("Epoll_wait failed");
        }

        for (int i = 0; i < cnt; ++i) {
            if (events[i].data.fd == sender.get()) {
                bytes_read = recv(sender.get(), buf, BUF_SIZE, 0);

                if (bytes_read == -1) {
                    throw std::runtime_error("Recv failed");
                }

                if (bytes_read == 0) {
                    break;
                }

                for (int j = 0; j < bytes_read; ++j) {
                    std::cout << buf[j] << std::flush;
                }
                sum += bytes_read;
            }

            if (events[i].data.fd == 0) {
                if (!getline(std::cin, message)) {
                    process = false;
                    continue;
                }

                message.push_back('\n');
                send_all(sender.get(), message.data(), message.size());
                expected_sum += message.size();
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
        fd_wrapper sender = open_connection(address, port);
        send_and_receive(sender);
    } catch (std::runtime_error &e) {
        print_err(e.what());
        return EXIT_FAILURE;
    }

}

