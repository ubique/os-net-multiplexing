#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc != 3)
        printError("Excepted address and port");

    char *message = new char[bufferLen];
    char buffer[bufferLen];
    int opt = 1, mainSock, addrlen, clientSock, clients[NUMBER_OF_CLIENTS] = {0}, curSock, maxSocketDescriptor;

    struct sockaddr_in address;
    char *addr = argv[1];
    address.sin_family = AF_INET;
    if (inet_pton(AF_INET, addr, &(address.sin_addr.s_addr)) <= 0)
        printSysError("inet_pton");
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    auto port = strtol(argv[2], nullptr, 10);
    address.sin_port = htons(static_cast<uint16_t>(port));
    fd_set clientsSet;
    if (errno == ERANGE || port > UINT16_MAX || port <= 0)
        printError("Number of port should be uint16_t");

    if ((mainSock = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
        printSysError("socket");

    if (setsockopt(mainSock, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0)
        printSysError("setsockopt");

    if (bind(mainSock, (struct sockaddr *) &address, sizeof(address)) < 0) {
        sprintf(message, "Bind error %d", errno);
        printError(message);
    }

    if (listen(mainSock, NUMBER_OF_CLIENTS) < 0)
        printSysError("listen");

    addrlen = sizeof(address);
    int incorrectAnswerHolder = -1;
    while (true) {
        FD_ZERO(&clientsSet);
        FD_SET(mainSock, &clientsSet);
        maxSocketDescriptor = mainSock;

        for (int &client : clients) {
            curSock = client;
            if (curSock > 0)
                FD_SET(curSock, &clientsSet);
            if (curSock > maxSocketDescriptor)
                maxSocketDescriptor = curSock;
        }

        processActivity(select(maxSocketDescriptor + 1, &clientsSet, nullptr, nullptr, nullptr));
        if (FD_ISSET(mainSock, &clientsSet)) {
            if ((clientSock = accept(mainSock, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0)
                printSysError("accept");
            sprintf(message, "Client %d connected (address: %s)\n", clientSock, inet_ntoa(address.sin_addr));
            writeStr(message);
            sendAll(clientSock, mess);
            for (int &client : clients)
                if (client == 0) {
                    client = clientSock;
                    break;
                }
        }

        for (int &client : clients) {
            curSock = client;
            if (FD_ISSET(curSock, &clientsSet)) {
                int result = sockRead(curSock, buffer);
                if (getpeername(curSock, (struct sockaddr *) &address, (socklen_t *) &addrlen) == -1)
                    printSysError("getpeername");
                closeSocket(curSock);
                client = 0;
//                std::cout << result << '\n';
                buffer[result - 1] = '\n';
                if (result >= 0)
                    if (strcmp(buffer, pali) == 0)
                        sprintf(message, "%d: Palindrome is built correctly\n", curSock);

                    else {
                        sprintf(message, "%d: Palindrome is built wrong\n", curSock);
                        incorrectAnswerHolder = curSock;
                    }

                else
                    sprintf(message, "%d: no action :(\n", curSock);

                writeStr(message);
            }
        }

        if (incorrectAnswerHolder != -1) {
            for (int &client : clients)
                close(client);
            closeSocket(mainSock);
            sprintf(message, "Client %d couldn't build palindrome\n", incorrectAnswerHolder);
            writeStr(message);
            break;
        }
    }

    delete[]message;
    return 0;
}
