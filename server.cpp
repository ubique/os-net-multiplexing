//
// Created by Yaroslav on 04/06/2019.
//

#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "utils.h"

using namespace std;

void wrong_usage() {
    cout << "Usage: ./Task5_client [address]";
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        wrong_usage();
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof serv_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    check_error(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr),"inet_pton");
    int master = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int true_var = 1;
    check_error(setsockopt(master, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &true_var, sizeof true_var),"setsockopt");
    check_error(bind(master, (struct sockaddr*)  &serv_addr, sizeof serv_addr),"bind");
    check_error(listen(master, 10),"listen");
    int epollCreate = epoll_create(1);
    struct epoll_event event, events[MAX_EPOLL_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = master;
    check_error(epoll_ctl(epollCreate, EPOLL_CTL_ADD, master, &event),"epoll_ctl");
    while (true) {
        int epollWait = epoll_wait(epollCreate, events, MAX_EPOLL_EVENTS, -1);
        check_error(epollWait,"Waiting for output failed");
        for (int i = 0; i < epollWait; i++) {
            int current = events[i].data.fd;
            if (current == master) {
                struct sockaddr_in cli_addr;
                socklen_t addrlen = sizeof cli_addr;
                int new_fd = accept(master, (struct sockaddr*) &cli_addr, &addrlen);;
                if (new_fd < 0) {
                    perror("Accept error");
                    continue;
                }


                int flags = fcntl(new_fd, F_GETFL, 0);
                flags |= O_NONBLOCK;
                fcntl(new_fd, F_SETFL, flags);
                event.data.fd = new_fd;
                event.events = EPOLLIN | EPOLLOUT;
                if (epoll_ctl(epollCreate, EPOLL_CTL_ADD, new_fd, &event) < 0) {
                    perror("Cannot add descriptor to epoll");
                    if (close(new_fd) < 0)
                        perror("Cannot close descriptor");
                }
            } else {
                if ((events[i].events & EPOLLIN) && (events[i].events & EPOLLOUT)) {
                    char buf[BUF_SIZE + 1];
                    fun_recv(current,buf,"server");
                    string message;
                    message = string(buf);
                    for (auto& i: message) {
                        i++;
                    }
                    fun_send(current, const_cast<char*>(message.data()),message.length(), "server");
                    event.data.fd = current;
                    check_non_stop(epoll_ctl(epollCreate, EPOLL_CTL_DEL, current, &event), "epoll_ctl");
                    check_non_stop(close(current), "close");
                    if (strcmp(buf, "exit") == 0)
                        return 0;
                }
            }
        }
    }
}