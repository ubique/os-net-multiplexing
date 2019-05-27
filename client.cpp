#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <Utils.h>
#include <fcntl.h>
#include <sys/epoll.h>

void print_error(const std::string &msg) {
    perror(msg.c_str());
    exit(EXIT_FAILURE);
}

int epoll_ctl_wrap(int epool_fd, int op, int fd, uint32_t events) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(epool_fd, op, fd, &event) == -1) {
        print_error("Can't make epoll_ctl");
        return -1;
    }
    return 0;
}

int delete_socket(int epoll, int fd) {
    if (epoll_ctl_wrap(epoll, EPOLL_CTL_DEL, fd, 0) == -1) {
        print_error("Can't delete socket from poll");
        return -1;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 0 || argc > 2) {
        std::cerr << "Incorrect count of arguments" << std::endl;
    }

    const char* host_name = argc == 1 ? argv[0] : argv[1];
    struct hostent *server;
    int socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socket_fd == -1) {
        print_error("ERROR opening socket");
        return 0;
    }
    fcntl(0, F_SETFL, O_NONBLOCK);
    server = gethostbyname(host_name);
    if (server == nullptr) {
        print_error("ERROR, no such host");
        close(socket_fd);
        return 0;
    }

    struct sockaddr_in server_addr{};
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(reinterpret_cast<char*>(&server_addr.sin_addr.s_addr), static_cast<char*>(server->h_addr), server->h_length);
    server_addr.sin_port = htons(8888);

    int status = connect(socket_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
    for (int i = 0; i < 32; i++) {
        if (status != -1 || (errno != EINTR)) {
            break;
        }
        status = connect(socket_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
    }

    int epoll_fd = epoll_create(8);
    if (epoll_fd == -1) {
        std::cerr << "Can't create epoll" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (epoll_ctl_wrap(epoll_fd, EPOLL_CTL_ADD, socket_fd, EPOLLIN | EPOLLOUT) == -1) {
        std::cerr << "Can't add socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (epoll_ctl_wrap(epoll_fd, EPOLL_CTL_ADD, 0, EPOLLIN) == -1) {
        std::cerr << "Can't add a stdin" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Client started!" << std::endl;
    std::cout << "For exit print \"exit\"" << std::endl;

    struct epoll_event events[8];
    while(true) {
        int count = epoll_wait(epoll_fd, events, 8, -1);
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        for(size_t i = 0; i < count; i++) {
            if (events[i].data.fd == 0) {
                if (events[i].events & EPOLLIN) {
                    std::string command;
                    std::getline(std::cin , command);
                    if (command == "exit") {
                        close(socket_fd);
                        exit(EXIT_SUCCESS);
                    }
                    if(send(socket_fd, command.c_str(), command.size() + 1, 0) == -1) {
                        print_error("bad!");
                        close(socket_fd);
                    }
                }
            } else if (events[i].data.fd == socket_fd) {
                if (events[i].events % EPOLLIN) {
                    int size_buf = 1024;
                    char buf[size_buf];
                    memset(buf, 0, sizeof(buf));
                    if (recv(socket_fd, &buf, size_buf, 0) == -1) {
                        print_error("Can't read!");
                    }
                    std::cout << "Response: " << buf << std::endl;
                }
            }
        }
    }
}