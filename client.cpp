#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "descriptor.h"

using namespace std;

const size_t BUF_SIZE = 100;
const size_t MAX_EPOLL_EVENTS = 100;

void print_error_and_exit(string message) {
    perror(message.c_str());
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        cout << "Usage: client <addr> <port> <name>\n";
        exit(EXIT_FAILURE);
    }
    string name = argv[3];
    for (int i = 4; i < argc; i++) {
        name += ' ';
        name += argv[i];
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof serv_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    if (!inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)) {
        cout << "Bad address " << argv[1] << '\n';
        exit(EXIT_FAILURE);
    }
    descriptor sockfd(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0));
    if (connect(sockfd, (struct sockaddr*)  &serv_addr, sizeof serv_addr) < 0 &&
        errno != EINPROGRESS)
        print_error_and_exit("Cannot connect to socket");


    descriptor epfd(epoll_create(1));
    struct epoll_event event, events[MAX_EPOLL_EVENTS];
    event.events = EPOLLOUT;
    event.data.fd = sockfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event) < 0)
        print_error_and_exit("Cannot add socket descriptor to epoll");
    int num_ready = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, -1);
    if (num_ready < 0)
        print_error_and_exit("Waiting for output failed");
    int i = 0;
    for (; i < num_ready; i++) {
        if (events[i].data.fd == sockfd && (events[i].events & EPOLLOUT)) {
            if (send(sockfd, name.data(), name.size(), 0) < 0)
                print_error_and_exit("Sending failed");
            break;
        }
    }
    if (i == num_ready)
        print_error_and_exit("Cannot send data to server");

    event.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event) < 0)
        print_error_and_exit("Cannot modify socket descriptor in epoll");
    num_ready = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, -1);
    if (num_ready < 0)
        print_error_and_exit("Waiting for input failed");
    for (; i < num_ready; i++) {
        if (events[i].data.fd == sockfd && (events[i].events & EPOLLIN)) {
            char buf[BUF_SIZE + 1];
            int sz = recv(sockfd, buf, BUF_SIZE, 0);
            if (sz < 0)
                print_error_and_exit("Receiving failed");
            buf[sz] = '\0';
            cout << buf << '\n';
            break;
        }
    }
    if (i == num_ready)
        print_error_and_exit("Cannot receive data from server");

    return 0;
}
