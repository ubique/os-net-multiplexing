#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define MAX_CONNECTIONS_QUEUE_LENGTH (10000)
#define EVENT_BUF_SIZE (32)
#define BUF_SIZE (1024)

#define FATAL_ERROR(msg) {perror(msg);exit(EXIT_FAILURE);}

void client_add_fd_to_epoll(const int epfd, const int fd, const int outFlag) {
    struct epoll_event epoll_struct;
    epoll_struct.events = EPOLLIN | EPOLLET | ((outFlag) ? EPOLLOUT : 0);
    epoll_struct.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epoll_struct) < 0) FATAL_ERROR("Failed to add file descriptor to epoll.")
}

void client_loop(const int sfd) {
    int epfd;
    if ((epfd = epoll_create(MAX_CONNECTIONS_QUEUE_LENGTH)) < 0) FATAL_ERROR("Failed to create epoll fd.")
    client_add_fd_to_epoll(epfd, sfd, 1);
    client_add_fd_to_epoll(epfd, STDIN_FILENO, 0);

    int wrote = 0;
    int ret = 0;
    int mlen = 0;
    int writeMode = 0;
    int connected = 0;
    char buffer[BUF_SIZE];
    char message[BUF_SIZE];
    int evc = 0;
    struct epoll_event events[EVENT_BUF_SIZE];
    int cont = 1;
    while (cont) {
        if ((evc = epoll_wait(epfd, events, EVENT_BUF_SIZE, -1)) < 0) FATAL_ERROR("Epoll wait failed.")
        for (int i = 0; i < evc; ++i) {
            if ((events[i].events & EPOLLOUT) == 0) {
                if (events[i].data.fd == STDIN_FILENO) {
                    if (writeMode == 0) {
                        fgets(message, BUF_SIZE, stdin);
                        mlen = strlen(message);
                        if (strcmp(message, ":q\n") == 0) {
                            cont = 0;
                            break;
                        }
                        wrote = 0;
                        ret = 0;
                        writeMode = 1;
                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, sfd, NULL) < 0) {
                            fprintf(stderr, "Unable delete multiplexer.\n");
                            exit(EXIT_FAILURE);
                        }
                        client_add_fd_to_epoll(epfd, sfd, 1);
                    } else {
                        char tmp[BUF_SIZE];
                        fgets(tmp, BUF_SIZE, stdin);
                    }
                }
            } else {
                if (!connected) {
                    int err = 0;
                    int errlen = 0;
                    if (getsockopt(sfd, SOL_SOCKET, SO_ERROR, (void*) &err, &errlen) != 0) FATAL_ERROR("")
                    if (err == 0) connected = 1;
                    continue;
                }
                while (wrote < mlen) {
                    int cur;
                    if ((cur = write(sfd, message + wrote, mlen - wrote)) < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        FATAL_ERROR("Write error.")
                    }
                    wrote += cur;
                }

                while (1) {
                    int rlen;
                    if ((rlen = recv(events[i].data.fd, buffer, BUF_SIZE, 0)) < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("Server is closed.");
                        cont = 0;
                        break;
                    }
                    if (rlen == 0) {
                        printf("Server disconnected\n");
                        cont = 0;
                        break;
                    }
                    buffer[rlen] = '\0';
                    printf("Recieved:%s\n", buffer);
                    ret += rlen;
                }
                if (ret == mlen) {
                    writeMode = 0;
                }
            }
        }
    }
    close(epfd);
    close(sfd);
}

int client_connect_to_server(const char* ip, const char* port) {
    int sfd; // server fd
    if ((sfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) < 0) FATAL_ERROR(
            "Failed to create socket.")

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    switch (inet_pton(AF_INET, ip, &server_addr.sin_addr)) {
        case -1: FATAL_ERROR("Failed to set ip.")
        case 0:
            fprintf(stderr, "Wrong ip address: '%s'\n", ip);
            exit(EXIT_FAILURE);
        default: // SUCCESS
            break;
    }

    if (connect(sfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
        if (errno != EINPROGRESS) FATAL_ERROR("Failed to connect to server.")
    return sfd;
}

void client_check_args(const int argc, const char* prog) {
    if (argc == 3) return;
    fprintf(stderr, "Usage: %s <IPv4> <PORT>\n", prog);
    exit(EXIT_FAILURE);
}

int main(int argc, const char* argv[]) {
    client_check_args(argc, argv[0]);
    client_loop(client_connect_to_server(argv[1], argv[2]));
    exit(EXIT_SUCCESS);
}
