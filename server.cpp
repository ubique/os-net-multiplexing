#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
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
    if (argc < 3) {
        cout << "Usage: server <addr> <port>\n";
        exit(EXIT_FAILURE);
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
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof yes) < 0)
        print_error_and_exit("Cannot reuse addr");
    if (bind(sockfd.fd, (struct sockaddr*)  &serv_addr, sizeof serv_addr) < 0)
        print_error_and_exit("Cannot bind to socket");
    if (listen(sockfd, 10) < 0)
        print_error_and_exit("Listening failed");

    descriptor epfd(epoll_create(1));
    struct epoll_event event, events[MAX_EPOLL_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = sockfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event) < 0)
        print_error_and_exit("Cannot add socket descriptor to epoll");

    while (true) {
        int num_ready = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, -1);
        if (num_ready < 0)
            print_error_and_exit("Waiting for output failed");
        for (int i = 0; i < num_ready; i++) {
            int cur_fd = events[i].data.fd;
            if (cur_fd == sockfd) {
                struct sockaddr_in cli_addr;
                socklen_t addrlen = sizeof cli_addr;
                int new_fd = accept(sockfd, (struct sockaddr*) &cli_addr, &addrlen);
                if (new_fd < 0) {
                    perror("Accepting connection failed");
                    continue;
                }

                int flags = fcntl(new_fd, F_GETFL, 0);
                flags |= O_NONBLOCK;
                fcntl(new_fd, F_SETFL, flags);
                event.data.fd = new_fd;
                event.events = EPOLLIN | EPOLLOUT;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &event) < 0) {
                    perror("Cannot add descriptor to epoll");
                    if (close(new_fd) < 0)
                        perror("Cannot close descriptor");
                }
            } else {
                if ((events[i].events & EPOLLIN) && (events[i].events & EPOLLOUT)) {
                    char buf[BUF_SIZE + 1];
                    int recv_cnt = 0;
                    int cnt;
                    while ((cnt = recv(cur_fd, buf + recv_cnt, BUF_SIZE - recv_cnt, 0)) > 0) {
                        recv_cnt += cnt;
                        if (buf[recv_cnt - 1] == '\n')
                            break;
                    }
                    if (cnt < 0) {
                        perror("Receiving failed");
                        continue;
                    }

                    buf[recv_cnt - 1] = '\0';
                    string message;
                    if (strcmp(buf, "close") != 0)
                        message = "Hello, " + string(buf) + "!\n";
                    else
                        message = "Bye!\n";
                    int send_cnt = 0;
                    cnt = 1;
                    while (send_cnt != message.size() && cnt > 0) {
                        cnt = send(cur_fd, message.data() + send_cnt, message.size() - send_cnt, 0);
                        send_cnt += cnt;
                    }
                    if (cnt < 0) {
                        perror("Sending failed");
                        continue;
                    }

                    event.data.fd = cur_fd;
                    if (epoll_ctl(epfd, EPOLL_CTL_DEL, cur_fd, &event) < 0)
                        perror("Cannot delete descriptor from epoll");
                    if (close(cur_fd) < 0)
                        perror("Cannot close descriptor");
                    if (strcmp(buf, "close") == 0)
                        return 0;
                }
            }
        }
    }
}
