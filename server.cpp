#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include "utils.h"

using std::cout;
using std::stoi;
using std::cerr;
using std::string;
using std::vector;

void get_args(char *argv[], string &host, int &port) {
    try {
        port = stoi(argv[2]);
    } catch (...) {
        cerr << "Invalid port";
        exit(EXIT_FAILURE);
    }
    host = argv[1];
}

void transform_msg(string &msg) {
    if (msg.empty())
        return;
    msg = "Z" + msg.substr(1, msg.size() - 1);
}

const int CONNECTIONS_TOTAL = 64;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cout << "Usage: ./os-net-server [host] [port]";
        exit(EXIT_FAILURE);
    }

    int port;
    string host;
    get_args(argv, host, port);

    //creating socket file descriptor
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    utils::check(fd, "in socket");

    //initializing server
    struct sockaddr_in server{}, client{};

    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    socklen_t s_len = sizeof(sockaddr_in);
    char buffer_size;
    string buffer;

    utils::check(inet_pton(AF_INET, host.data(), &server.sin_addr), "in inet_pton");
    utils::check(bind(fd, (sockaddr *) (&server), s_len), "in bind");
    utils::check(listen(fd, SOMAXCONN), "in listen");

    //let's create epoll descriptor
    struct epoll_event epoll_event{}, events[CONNECTIONS_TOTAL];
    int ed = epoll_create1(0);

    //1
    epoll_event.events = EPOLLIN;
    epoll_event.data.fd = 0;
    utils::check(epoll_ctl(ed, EPOLL_CTL_ADD, 0, &epoll_event), "epoll_ctl");

    //2
    epoll_event.events = EPOLLIN;
    epoll_event.data.fd = fd;
    utils::check(epoll_ctl(ed, EPOLL_CTL_ADD, fd, &epoll_event), "epoll_ctl");

    //running...
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    cout << "Running on port " << port << std::endl;
    while (true) {
        int descriptors = epoll_wait(ed, events, CONNECTIONS_TOTAL, -1);
        utils::check(descriptors, "in epoll_wait", true);

        for (int i = 0; i < descriptors; i++) {
            int descriptor = events[i].data.fd;
            if (descriptor == fd) {
                struct sockaddr_in client_addr{};
                socklen_t address_size = sizeof(client_addr);

                int ad = accept(fd, (struct sockaddr *) &client_addr, &address_size);
                if (ad == -1) {
                    perror("bad accept descriptor");
                    continue;
                }

                cout << "new connection\n";
                int flags = fcntl(ad, F_GETFL, 0);
                fcntl(ad, F_SETFL, flags | O_NONBLOCK);

                epoll_event.events = EPOLLIN | EPOLLET;
                epoll_event.data.fd = ad;
                utils::check(epoll_ctl(ed, EPOLL_CTL_ADD, ad, &epoll_event), "in epoll_ctl", true);
            } else {
                utils::receive_msg(&buffer_size, 1, descriptor);
                buffer.clear();
                buffer.resize(buffer_size, 0);
                utils::receive_msg(&buffer[0], buffer_size, descriptor);
                utils::print_msg(buffer);

                transform_msg(buffer);
                utils::send_msg(&buffer[0], buffer_size, descriptor);

                utils::check(shutdown(descriptor, SHUT_RDWR), "in client descriptor shutdown", true);
                utils::check(close(descriptor), "in client descriptor close", true);
            }
        }
    }
#pragma clang diagnostic pop
}