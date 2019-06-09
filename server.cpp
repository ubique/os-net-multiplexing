#include "utils.h"
#include <unordered_map>
#define MAX_EVENTS 1000

void resolve(const std::string& addr, const std::string& port) {
    int sockfd;
    int epoll;
    struct sockaddr_in server;
    struct epoll_event ev, events[MAX_EVENTS];

    utils::epoll_create(epoll);
    utils::init(sockfd, server, addr, port, SOCK_STREAM | SOCK_NONBLOCK);
    utils::bind_and_listen(sockfd, server);
    if (!utils::add(epoll, sockfd, &ev)) {
        exit(EXIT_FAILURE);
    }
    printf("Waiting for connections ...\n");

    while (true) {
        int nfds = epoll_wait(epoll, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("Cannot wait more");
            break;
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == sockfd) {
                struct sockaddr_in user;
                socklen_t len = sizeof(user);
                int resultfd;
                if (!utils::accept(resultfd, sockfd, reinterpret_cast<sockaddr*>(&user), &len)) {
                    continue;
                }
                printf("%s connected\n", inet_ntoa(user.sin_addr));
                utils::add(epoll, resultfd, &ev);
            } else {
                int resultfd = events[i].data.fd;
                char buffer[utils::BUF_SIZE];
                if (!utils::recv(resultfd, buffer, 1)) {
                    utils::abort(epoll, resultfd);
                    continue;
                }
                char size = buffer[0];
                if (!utils::recv(resultfd, buffer, size)) {
                    utils::abort(epoll, resultfd);
                    continue;
                }
                printf("Message: %s\n", buffer);
                if (!utils::send(resultfd, &size, 1)) {
                    utils::abort(epoll, resultfd);
                    continue;
                }
                if (!utils::send(resultfd, buffer, size)) {
                    utils::abort(epoll, resultfd);
                    continue;
                }
                printf("Echo message was sent\n");
            }
        }
    }
    utils::close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s [addr] [port]\n", argv[0]);
        return 0;
    }
    resolve(argv[1], argv[2]);
}
