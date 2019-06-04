#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define MAX_CONNECTIONS_QUEUE_LENGTH (10000)
#define EVENT_BUF_SIZE (32)
#define BUF_SIZE (1024)

#define FATAL_ERROR(msg) {perror(msg);exit(EXIT_FAILURE);}

void client_add_fd_to_epoll(const int epfd, const int fd) {
    struct epoll_event epoll_struct;
    epoll_struct.events = EPOLLIN | EPOLLET;
    epoll_struct.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epoll_struct) < 0)
        fprintf(stderr, "Failed to add file descriptor to epoll.\n");
}

int client_handle_input(const int sfd) {
    char data[BUF_SIZE];
    fgets(data, BUF_SIZE, stdin);
    if (strcmp(data, ":q\n") == 0) return 1;
    if (write(sfd, data, strlen(data)) < 0) FATAL_ERROR("Failed to send message.")
    return 0;
}

int client_handle_server_answer(const int sfd) {
    int readl;
    char data[BUF_SIZE];
    if ((readl = recv(sfd, data, BUF_SIZE - 1, 0)) < 0) FATAL_ERROR("Failed to read answer from server.")
    if (readl == 0) {
        printf("Server disconnected.\n");
        return 1;
    }
    data[readl] = '\0';
    printf("Received: %s", data);
    return 0;
}

void client_loop(const int sfd) {
    printf("For finishing session enter ':q'.");
    int epfd;
    if ((epfd = epoll_create(MAX_CONNECTIONS_QUEUE_LENGTH)) < 0) FATAL_ERROR("Failed to create epoll fd.")
    client_add_fd_to_epoll(epfd, sfd);
    client_add_fd_to_epoll(epfd, STDIN_FILENO);

    int evc = 0;
    struct epoll_event events[EVENT_BUF_SIZE];
    int cont = 1;
    while (cont) {
        if ((evc = epoll_wait(epfd, events, EVENT_BUF_SIZE, -1)) < 0) FATAL_ERROR("Epoll wait failed.")
        for (int i = 0; i < evc; ++i) {
            if (events[i].data.fd == STDIN_FILENO) {
                if (client_handle_input(sfd) == 1) cont = 0;
            } else if (events[i].data.fd == sfd) {
                if (client_handle_server_answer(sfd) == 1) cont = 0;
            }
        }
    }
    close(sfd);
    close(epfd);
}

int client_connect_to_server(const char* ip, const char* port) {
    int sfd; // server fd
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) FATAL_ERROR("Failed to create socket.")

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

    if (connect(sfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) FATAL_ERROR(
            "Failed to connect to server.")

    return sfd;
}

void client_check_args(const int argc, const char* prog) {
    if (argc == 3) return;
    fprintf(stderr, "Usage: %s <IPv4> <PORT>\n", prog);
    exit(EXIT_FAILURE);
}

int main(int argc, const char* argv[]) {
    client_check_args(argc, argv[0]);
    printf("For finishing session enter ':q'.");
    client_loop(client_connect_to_server(argv[1], argv[2]));
    exit(EXIT_SUCCESS);
}
