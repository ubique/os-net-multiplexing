#include "utils.h"

int clients[NUMBER_OF_CLIENTS];

void workWithServer(char * buffer, char * message) {
    int curSock, maxSocketDescriptor, pool = 0;
    fd_set clientsSet{};
//    srand(time(0)); // to select malfunctioning client
    while (pool != NUMBER_OF_CLIENTS) {
        __FD_ZERO(&clientsSet);
        maxSocketDescriptor = clients[0];
        for (auto &client : clients) {
            curSock = client;
            if (curSock > 0)
                __FD_SET(curSock, &clientsSet);
            if (curSock > maxSocketDescriptor)
                maxSocketDescriptor = curSock;
        }
        processActivity(select(maxSocketDescriptor + 1, &clientsSet, nullptr, nullptr, nullptr));
        for (int &client : clients) {
            curSock = client;
            if (FD_ISSET(curSock, &clientsSet)) {
                int res;
                if ((res = sockRead(curSock, buffer)) < 0)
                    printError("Reading failed\n");
                if (rand() % NUMBER_OF_CLIENTS != 0)
                    buffer = proccessPalindrome(buffer, res);
                buffer[res] = '\n';
                sprintf(message, "Client %d trying to reply\n", curSock);
                writeStr(message);
                sendAll(curSock, buffer);
                sprintf(message, "Client %d replied\n", curSock);
                writeStr(message);
                closeSocket(curSock);
                client = 0;
                ++pool;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3)
        printError("Expected: address and port\n");

    char *address = argv[1];
    char *buffer = new char[bufferLen];
    char *message = new char[bufferLen];

    auto port = strtol(argv[2], nullptr, 10);
    if (errno == ERANGE || port > UINT16_MAX || port <= 0)
        printError("Number of port should be uint16_t");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, address, &(addr.sin_addr.s_addr)) <= 0)
        printSysError("inet_pton");
    addr.sin_port = htons(static_cast<uint16_t>(port));

    for (int & client : clients) {
        int sock;
        if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
            printSysError("socket");

        connect(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));

        struct timeval timeout;
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout)) < 0)
            printSysError("setsockopt");

        sprintf(message, "Client %d connected.\n", sock);
        writeStr(message);
        client = sock;
    }

    workWithServer(buffer, message);

    delete []buffer;
    delete []message;
    return 0;
}