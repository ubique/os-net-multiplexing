#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <sys/epoll.h>
#include "utils.h"

using std::cin;
using std::cout;
using std::stoi;
using std::cerr;
using std::string;
using std::vector;

const int CONNECTIONS_TOTAL = 64;

void get_args(char *argv[], string &host, int &port) {
    try {
        port = stoi(argv[2]);
    } catch (...) {
        cerr << "Invalid port\n";
        exit(EXIT_FAILURE);
    }
    host = argv[1];
}

const int BUFFER_SIZE = 42;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cout << "Usage: ./os-net-multiplexing-client [host] [port]";
        exit(EXIT_FAILURE);
    }

    int port;
    string host;
    int ed = epoll_create1(0);
    get_args(argv, host, port);

    //creating socket file descriptor
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    utils::check(fd, "in socket");

    struct sockaddr_in server{};

    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    socklen_t s_len = sizeof(sockaddr_in);

    utils::check(inet_pton(AF_INET, host.data(), &server.sin_addr), "in inet_pton");
    int connect_result = (connect(fd, (sockaddr *) (&server), sizeof(server)) == -1 && errno != EINPROGRESS) ? -1 : 0;
    utils::check(connect_result, "in connect");

    struct epoll_event ee{}, ee_1{};

    utils::add_epoll(ed, fd, &ee, EPOLLOUT, EPOLL_CTL_ADD);

    int descriptors = epoll_wait(ed, &ee_1, 1, -1);
    utils::check(descriptors, "in epoll_wait");

    vector<char> msg(BUFFER_SIZE);
    for (char& x: msg) {
        x = 'a';
    }
    msg.back() = '\0';

    if (!(ee.events & EPOLLOUT)) {
        return EXIT_FAILURE;
    }
    utils::send_msg(&msg[0], BUFFER_SIZE, fd);

    utils::add_epoll(ed, fd, &ee, EPOLLIN, EPOLL_CTL_MOD);

    descriptors = epoll_wait(ed, &ee_1, 1, -1);
    utils::check(descriptors, "in epoll_wait");

    if (!(ee.events & EPOLLIN)) {
        return EXIT_FAILURE;
    }

    utils::receive_msg(&msg[0], BUFFER_SIZE, fd);
    utils::print_msg(msg);
    utils::check(shutdown(fd, SHUT_RDWR), "in descriptor shutdown");
    utils::check(close(fd), "in descriptor close");
    return 0;
}