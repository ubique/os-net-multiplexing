#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc != 2)
        printError("Expected: port\n");

    char buffer[bufferLen];

    auto port = strtol(argv[1], nullptr, 10);
    if (errno == ERANGE || port > UINT16_MAX || port <= 0)
        printError("Number of port should be uint16_t");

    int epoll;
    struct epoll_event events[NUMBER_OF_EVENTS], event;
    if ((epoll = epoll_create1(0)) == -1)
        printSysError("epoll_create1");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printSysError("socket");

    if (connect(sock, (sockaddr *)(&addr), sizeof(addr)) == -1)
        printSysError("connect");

    printf("Client %d connected.\n", sock);

    epollAdding(epoll, sock, &event);
    epollAdding(epoll, 0, &event);

    char size;
    for (;;) {
        if (feof(stdin) != 0)
            return 0;

        int numfd;
        if ((numfd = epoll_wait(epoll, events, NUMBER_OF_EVENTS, -1)) == -1)
            printSysError("epoll_wait");

        for (int i = 0; i < numfd; ++i) {
            if (events[i].data.fd == sock) {
                recvAll(sock, buffer, 1, false);
                size = buffer[0];
                recvAll(sock, buffer, size, false);
                printf("Res: %s\n", buffer);
            } else {
                scanf("%255s", buffer);
                size = strlen(buffer);
                ++size;
                sendAll(sock, &size, 1, false);
                sendAll(sock, buffer, size, false);
            }
        }
    }
    closeSocket(sock);
    return 0;
}
