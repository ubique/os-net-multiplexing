#include <fcntl.h>
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

int fd;
std::deque<char> data;

int send(int fd, std::deque<char> &data) {
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

int receive(int fd, std::deque<char> &data) {
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
    my_close(socket, s.c_str());
}

//first argument ip, second argument port
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Wrong usage: expected two arguments!\n");
        return 0;
    }
    int client_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (client_socket == -1) {
        fprintf(stderr, "Can't create a socket: %s\n", strerror(errno));
        return 0;
    }
    int st = fcntl(0, F_SETFL, O_NONBLOCK); 
    if (st == -1) {
        fprintf(stderr, "Can't make stdin nonblocking: %s\n", strerror(errno));
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
    status = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
    for (int i = 0; i < tries; i++) {
        if (status != -1 || (errno != EINTR)) {
            break;
        }
        status = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
    }
    if (status == -1 && errno != EINPROGRESS) {
        if (errno == EADDRINUSE || errno == EISCONN || errno == EADDRNOTAVAIL) {
            fprintf(stderr, "Error while connecting: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "Can't connect client socket: %s\n", strerror(errno));
        }
        return 0;
    }
    bool inprogress = false;
    if (errno == EINPROGRESS) inprogress = true;
    int epoll = epoll_create(1);
    if (epoll == -1) {
        fprintf(stderr, "Can't create epoll: %s\n", strerror(errno));
        return 0;
    }
    struct epoll_event events[MAXEVENTS];
    if (inprogress == true) {
        int st = make_epoll_ctl(epoll, EPOLL_CTL_ADD, client_socket, EPOLLOUT);
        if (st == -1) {
            return 0;
        }
        while (int num = epoll_wait(epoll, events, MAXEVENTS, -1)) {
            if (num == -1) {
                if (errno == EINTR) continue;
                fprintf(stderr, "epoll_wait failed: %s\n", strerror(errno));
                break;    
            }
            int stat;
            socklen_t len = sizeof(int);
            int st = getsockopt(client_socket, SOL_SOCKET, SO_ERROR, &stat, &len);
            if (st == -1) {
                fprintf(stderr, "Can't getsockopt: %s\n", strerror(errno));
                return 0;
            }
            if (stat != 0) {
                fprintf(stderr, "Can't connect: %s\n", strerror(stat));
                return 0;
            }
            if (stat == 0) {
                break;
            }
        }
        st = make_epoll_ctl(epoll, EPOLL_CTL_DEL, client_socket, 0);
        if (st == -1) {
            return 0;
        }
    }
    
    status = make_epoll_ctl(epoll, EPOLL_CTL_ADD, client_socket, EPOLLIN | EPOLLERR);    
    if (status == -1) {
        return 0;
    }
    status = make_epoll_ctl(epoll, EPOLL_CTL_ADD, 0, EPOLLIN);
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
            if (events[q].data.fd == client_socket) {
                if (events[q].events & EPOLLERR) {
                    close_socket(epoll, client_socket, "client");
                    return 0;
                }
                int socket = client_socket;
                if (events[q].events & EPOLLIN) {
                    std::deque<char> data2;
                    int st = receive(socket, data2);
                    if (st == -1) continue;
                    st = send(1, data2);
                    if (st == -1) {
                        return 0;
                    }
                }
                if (events[q].events & EPOLLOUT) {
                    int szk = data.size();
                    int st = send(client_socket, data);
                    if (st == -1) {
                        return 0;
                    }
                    int szq = data.size();
                    if (szk > 0 && szq == 0) {
                        int st = make_epoll_ctl(epoll, EPOLL_CTL_MOD, client_socket, EPOLLIN | EPOLLERR);  
                        if (st == -1) {
                            return 0;
                        }
                    }
                }
            } else {
                int socket = 0;
                if (events[q].events & (EPOLLERR | EPOLLHUP)) {
                    close_socket(epoll, socket, "stdin");
                    return 0;
                }
                int szk = data.size();
                if (events[q].events & EPOLLIN) {
                    int status = receive(0, data);
                    if (status < 0) {
                        close_socket(epoll, socket, "stdin");
                        return 0;
                    }
                    int szq = data.size();
                    if (szk == 0 && szq) {
                        int status = make_epoll_ctl(epoll, EPOLL_CTL_MOD, client_socket, EPOLLIN | EPOLLOUT | EPOLLERR); 
                        if (status < 0) {
                            return 0;
                        }
                    }
                }

            }
        }   
    }
    close_socket(epoll, client_socket, "client");
    my_close(epoll, "epoll");
    return 0;
}