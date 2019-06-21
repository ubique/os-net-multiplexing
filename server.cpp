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

void transform_msg(vector<char>& msg) {
    if (msg.empty())
        return;
    msg[0] = 'Z';
    msg.back() = '\0';
}

const int CONNECTIONS_TOTAL = 64;
const int BUFFER_SIZE = 42;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cout << "Usage: ./os-net-multiplexing-server [host] [port]";
        exit(EXIT_FAILURE);
    }

    int port;
    string host;
    get_args(argv, host, port);

    //creating socket file descriptor
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    utils::check(fd, "in socket");

    //initializing server
    struct sockaddr_in server{}, client{};

    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    socklen_t s_len = sizeof(sockaddr_in);
    vector<char> buffer(BUFFER_SIZE);

    utils::check(inet_pton(AF_INET, host.data(), &server.sin_addr), "in inet_pton");
    utils::check(bind(fd, (sockaddr *) (&server), s_len), "in bind");
    utils::check(listen(fd, SOMAXCONN), "in listen");

    //let's create epoll descriptor
    struct epoll_event ee{}, epollout_event{}, events[CONNECTIONS_TOTAL];
    int ed = epoll_create1(0);
    int descriptors;
    utils::add_epoll(ed, fd, &ee, EPOLLIN, EPOLL_CTL_ADD);

    //running...
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    cout << "Running on port " << port << std::endl;
    while (true) {
        descriptors = epoll_wait(ed, events, CONNECTIONS_TOTAL, -1);
        utils::check(descriptors, "in epoll_wait", true);

        for (int i = 0; i < descriptors; i++) {
            int e_fd = events[i].data.fd;
            if (e_fd == fd) {
                epoll_event ee_1{};
                utils::handle_new_connection(fd, ed, &ee_1);
            } else {
                if (events[i].events & EPOLLIN) {
                    utils::receive_msg(&buffer[0], BUFFER_SIZE, e_fd);
                    utils::print_msg(buffer);

                    transform_msg(buffer);
                    struct epoll_event ee_1{};

                    utils::add_epoll(ed, e_fd, &ee_1, EPOLLOUT, EPOLL_CTL_MOD);
                }
                if (events[i].events & EPOLLOUT) {
                    utils::send_msg(&buffer[0], BUFFER_SIZE, e_fd);
                    utils::check(shutdown(e_fd, SHUT_RDWR), "in client descriptor shutdown", true);
                    utils::check(close(e_fd), "in client descriptor close", true);
                }
            }
        }
    }
#pragma clang diagnostic pop
}