#include "utils.h"
#include <cstring>
#include <stdexcept>
#define MAX_EVENTS 1000

void resolve(const std::string& addr, const std::string& port) {
    int sockfd;
    int epoll;
    struct sockaddr_in server;
    struct epoll_event ev, events[MAX_EVENTS];

    utils::epoll_create(epoll);
    utils::init(sockfd, server, addr, port, SOCK_STREAM);
    utils::connect(sockfd, server);
    printf("Connected to server\n");
    if (!utils::add(epoll, sockfd, &ev) || !utils::add(epoll, 0, &ev)) {
        exit(EXIT_FAILURE);
    }

    while (true) {
        if (feof(stdin) != 0) {
            printf("\n");
            exit(EXIT_SUCCESS);
        }
        int nfds = epoll_wait(epoll, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("Cannot wait more");
            break;
        }
        char buffer[utils::BUF_SIZE];
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == sockfd) {
                if (!utils::recv(sockfd, buffer, 1)) {
                    printf("Aborting connection\n");
                    exit(EXIT_FAILURE);
                }
                char size = buffer[0];
                if (!utils::recv(sockfd, buffer, size)) {
                    printf("Aborting connection\n");
                    exit(EXIT_FAILURE);
                }
                printf("%s\n", buffer);
            } else {
                scanf("%255s", buffer);
                char size = strlen(buffer) + 1;
                if (!utils::send(sockfd, &size, 1)) {
                    printf("Aborting connection\n");
                    exit(EXIT_FAILURE);
                }
                if (!utils::send(sockfd, buffer, size)) {
                    printf("Aborting connection\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    utils::close(sockfd);
    printf("Disconnected from server\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s [addr] [port]\n", argv[0]);
        return 0;
    }
    resolve(argv[1], argv[2]);
}

