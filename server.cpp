#include <iostream>
#include <memory>
#include <map>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <deque>
#include <unistd.h>
#include <sys/epoll.h>

const int MAXEVENTS = 10;
const int CMAX = 2 * 1024, tries = 100;
char buf[CMAX + 1];

struct my_connect {
    int fd;
    std::deque<char> data;
    my_connect() {
        fd = -1;
        data.resize(0);
    }
    my_connect(int _fd) {
        fd = _fd;
        data.resize(0);
    }
    
    int send() {
        while (data.size()) {
            int sum = 0, len = std::min(CMAX, (int)data.size());
            for (int i = 0; i < len; i++) {
                buf[i] = data[i];
            }
            while (len) {
                int cnt = write(fd, buf + sum, len);
                if (cnt == 0) break;
                if (cnt == -1) {
                    if (errno == EAGAIN || errno == EINTR) {
                        return 0;
                    }
                    fprintf(stderr, "Can't write: %s\n", strerror(errno));
                    return -1;
                }
                len -= cnt;
                sum += cnt;
            }
            for (int j = 0; j < sum; j++) data.pop_front();
        }
        return 0;
    }

    int receive() {
        int kk = 0;
        while (int cnt = read(fd, buf, CMAX)) {
            kk++;
            if (cnt == -1) {
                if (errno == EINTR || errno == EAGAIN) {
                    return 0;
                }
                fprintf(stderr, "Can't read: %s\n", strerror(errno));
                return -1;
            }
            for (int j = 0; j < cnt; j++) {
                data.push_back(buf[j]);
            }
        }
        if (kk == 0) return -2;
        return 0;
    }
};

std::map<int, my_connect> connections;

short parse(const char *argv, const char *name) {
    short ans = 0;
    for (int i = 0; argv[i] != '\0'; i++) {
        if (argv[i] < '0' || argv[i] > '9') {
            fprintf(stderr, "%s must be a natural number\n", name);
            exit(0);
        }
        ans = 10 * ans + (argv[i] - '0');
    }
    return ans;
}

void my_close(int socket, const char *name) {
    int status = close(socket);
    for (int i = 0; i < tries; i++) {
        if (status != -1 || errno != EINTR) {
            break;
        }
        status = close(socket);
    }
    if (status == -1) {
        fprintf(stderr, "Can't close %s socket: %s\n", name, strerror(errno));
        exit(0);
    }
}

int make_epoll_ctl(int epoll, int op, int socket, uint32_t events) {
    struct epoll_event event;
    event.data.fd = socket;
    event.events = events;
    int status = epoll_ctl(epoll, op, socket, &event);
    if (status == -1) {
        fprintf(stderr, "Can't make epoll_ctl: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void close_socket(int epoll, int socket, std::string s) {
    int status = make_epoll_ctl(epoll, EPOLL_CTL_DEL, socket, 0);
    if (status == -1) {
        exit(0);
    }
    connections.erase(socket);
    my_close(socket, s.c_str());
}

//first argument ip, second argument port
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Wrong usage: expected two arguments!\n");
        return 0;
    }
    int server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_socket == -1) {
        fprintf(stderr, "Can't create a socket: %s\n", strerror(errno));
        return 0;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(parse(argv[2], "Port"));
    int status = inet_pton(AF_INET, argv[1], &addr.sin_addr.s_addr);
    if (status == 0) {
        fprintf(stderr, "%s isn't correct ipv4 adress\n", argv[1]);
        return 0;
    }
    if (status == -1) {
        fprintf(stderr, "Can't parse the ip: %s\n", strerror(errno));
        return 0;
    }
    status = bind(server_socket, (struct sockaddr *) &addr, sizeof(addr));
    if (status == -1) {
        if (errno == EADDRINUSE) {
            fprintf(stderr, "Error while binding: %s\n", strerror(errno));
        } else if (errno == EINVAL) {
            fprintf(stderr, "Error invalid address for bind: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "Can't bind address to server socket: %s\n", strerror(errno));
        }
        return 0;
    }
    status = listen(server_socket, 50);
    if (status == -1) {
        if (errno == EADDRINUSE) {
            fprintf(stderr, "Error while listening: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "Can't listen server socket: %s\n", strerror(errno));
        }
        return 0;
    }
    int epoll = epoll_create(1);
    if (epoll == -1) {
        fprintf(stderr, "Can't create epoll: %s\n", strerror(errno));
        return 0;
    }
    struct epoll_event events[MAXEVENTS];
    status = make_epoll_ctl(epoll, EPOLL_CTL_ADD, server_socket, EPOLLIN);    
    if (status == -1) {
        return 0;
    }
    while (int num = epoll_wait(epoll, events, MAXEVENTS, -1)) {
        if (num == -1) {
            if (errno == EINTR) continue;
            fprintf(stderr, "epoll_wait failed: %s\n", strerror(errno));
            break;
        }
        for (int q = 0; q < num; q++) {
            if (events[q].data.fd == server_socket) {
                int client_socket = accept4(server_socket, NULL, NULL, SOCK_NONBLOCK);
                if (client_socket == -1) {
                    if (errno != EAGAIN || errno != EINTR)
                        fprintf(stderr, "Can't accept: %s\n", strerror(errno));
                    continue;
                }
                status = make_epoll_ctl(epoll, EPOLL_CTL_ADD, client_socket, EPOLLIN | EPOLLERR | EPOLLHUP);    
                if (status == -1) {
                    my_close(client_socket, "client");
                    continue;
                }
                connections[client_socket] = my_connect(client_socket);
            } else {
                int client_socket = events[q].data.fd;
                if (events[q].events & (EPOLLERR | EPOLLHUP)) {
                    close_socket(epoll, client_socket, "client");
                    continue;
                }
                int szk = connections[client_socket].data.size();
                if (events[q].events & EPOLLIN) {
                    int status = connections[client_socket].receive();
                    if (status < 0) {
                        close_socket(epoll, client_socket, "client");
                        continue;
                    }
                }
                if (events[q].events & EPOLLOUT) {
                    int status = connections[client_socket].send();
                    if (status == -1) {
                        close_socket(epoll, client_socket, "client");
                        continue;
                    }
                }
                int szq = connections[client_socket].data.size();
                if (szk > 0 && szq == 0) {
                    status = make_epoll_ctl(epoll, EPOLL_CTL_MOD, client_socket, EPOLLIN | EPOLLERR | EPOLLHUP); 
                    if (status == -1) {
                        return 0;
                    }
                }
                if (szk == 0 && szq > 0) {
                    status = make_epoll_ctl(epoll, EPOLL_CTL_MOD, client_socket, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP);    
                    if (status == -1) {
                        return 0;
                    }
                }
            }
        }   
    }
    close_socket(epoll, server_socket, "server");
    my_close(epoll, "epoll");
    return 0;
}