//
// Created by Yaroslav on 04/06/2019.
//
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "utils.h"


void wrong_usage() {
    cout << "Usage: ./Task5_client [address] [message]";
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        wrong_usage();
        exit(EXIT_FAILURE);
    }
    string name = argv[2];
    name += '\n';
    if (name.size() > 80) {
        cout << "Second arg should be 80 symbols or shorter\n";
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof serv_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    check_error(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr), "inet_pton");
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (connect(sockfd, (struct sockaddr*)  &serv_addr, sizeof serv_addr) < 0 &&
        errno != EINPROGRESS) {
        check_error(-1,"Cannot connect to socket");
    }
    int epollCreate = epoll_create(1);
    struct epoll_event event, events[MAX_EPOLL_EVENTS];
    event.events = EPOLLOUT;
    event.data.fd = sockfd;
    check_error(epoll_ctl(epollCreate, EPOLL_CTL_ADD, sockfd, &event), "epoll_ctl");
    int epollWait = epoll_wait(epollCreate, events, MAX_EPOLL_EVENTS, -1);
    check_error(epollWait, "Waiting for output failed");
    int i = 0;
    for (; i < epollWait; i++) {
        if (events[i].data.fd == sockfd && (events[i].events & EPOLLOUT)) {
            if(fun_send_client(sockfd, const_cast<char *>(name.data()),name.length(),"client")) {
                check_error(-1,"Sending failed");
            }
            break;
        }
    }
    if (i == epollWait) {
        check_error(-1,"Cann't send data to server");
    }
    event.events = EPOLLIN;
    check_error(epoll_ctl(epollCreate, EPOLL_CTL_MOD, sockfd, &event), "epoll");
    epollWait = epoll_wait(epollCreate, events, MAX_EPOLL_EVENTS, -1);
    check_error(epollWait,"Waiting for input failed");
    for (; i < epollWait; i++) {
        if (events[i].data.fd == sockfd && (events[i].events & EPOLLIN)) {
            char buf[BUF_SIZE + 1];
            if(fun_recv_client(sockfd, buf, "client")) {
                check_error(-1,"Receiving failed");
            }
            cout << buf << '\n';
            break;
        }
    }
    if (i == epollWait) {
        check_error(-1,"Cannot receive data from server");
    }

    return 0;
}