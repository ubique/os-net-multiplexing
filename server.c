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

void server_add_fd_to_epoll(const int epfd, const int fd) {
    struct epoll_event ev_server;
    ev_server.events = EPOLLIN | EPOLLET;
    ev_server.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev_server) < 0) FATAL_ERROR("Failed to add file descriptor to epoll.")
}

void server_handle_new_client(const int epfd, const int sfd) {
    int cfd;
    if ((cfd = accept(sfd, (struct sockaddr*) NULL, NULL)) < 0) {
        fprintf(stderr, "Failed to accept new client.\n");
        return;
    }
    server_add_fd_to_epoll(epfd, cfd);
    printf("New client connected.\n");
}

void server_handle_client(const int cfd, const int epfd) {
    char data[BUF_SIZE];
    int readl;
    if ((readl = read(cfd, data, BUF_SIZE)) < 0) {
        perror("");
        return;
    }
    data[readl] = '\0';
    if (readl == 0) {
        printf("Client disconnected.\n");
        if (epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL) < 0) FATAL_ERROR("Unable delete multiplexer.")
    } else {
        int sent = 0;
        while (1) {
            int writel;
            if ((writel = write(cfd, data, readl)) < 0) perror("Failed to send message to client.");
            sent += writel;
            if (sent >= readl) {
                break;
            }
        }
    }
}

void server_loop(const int sfd) {
    int epfd; // epoll fd
    if ((epfd = epoll_create(MAX_CONNECTIONS_QUEUE_LENGTH)) < 0) FATAL_ERROR("Failed to create epoll fd.")
    server_add_fd_to_epoll(epfd, sfd);

    int evc;
    struct epoll_event events[EVENT_BUF_SIZE];
    while (1) {
        if ((evc = epoll_wait(epfd, events, EVENT_BUF_SIZE, -1)) < 0) FATAL_ERROR("Epoll wait failed.")
        for (int i = 0; i < evc; ++i) {
            if (events[i].data.fd == sfd) server_handle_new_client(epfd, sfd);
            else server_handle_client(events[i].data.fd, epfd);
        }
    }
    close(sfd);
    close(epfd);
}

int server_init(const char* ip, const char* port) {
    int sfd; // server fd
    if ((sfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) FATAL_ERROR("Failed to create socket.")

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

    if (bind(sfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) FATAL_ERROR("Failed to bind socket.")
    printf("Server bind to:\n\tip:%s\n\tport:%hu\n\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    if (listen(sfd, MAX_CONNECTIONS_QUEUE_LENGTH) < 0) FATAL_ERROR("Failed to listen socket.")
    return sfd;
}

void server_check_args(const int argc, const char* prog) {
    if (argc == 3) return;
    fprintf(stderr, "Usage: %s <IPv4> <PORT>\n", prog);
    exit(EXIT_FAILURE);
}

int main(int argc, const char* argv[]) {
    server_check_args(argc, argv[0]);
    server_loop(server_init(argv[1], argv[2]));
    exit(EXIT_SUCCESS);
}
