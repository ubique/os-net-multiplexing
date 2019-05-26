//
// Created by Noname Untitled on 25.05.19.
//
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <set>

#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>

#include "Server.h"

// Thanks CSC Course for this function
int set_nonblock(int fd) {
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

Server::Server(char *port) : mServerSocket(-1), mServerAddress(), mConnectedSockets() {
    mPort = std::stoi(port);

    if (mPort < MIN_PORT || mPort > MAX_PORT) {
        std::ostringstream errorMsg;
        errorMsg << "Port should be numeric value from " << MIN_PORT << " to " << MAX_PORT << "!\n";
        throw std::invalid_argument(errorMsg.str());
    }
}

Server::~Server() {
    if (mServerSocket != -1) {
        shutdown(mServerSocket, SHUT_RDWR);
        close(mServerSocket);
    }

    std::cout << "Terminate server!" << std::endl;
}

void Server::createBinding() {
    mServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mServerSocket == -1) {
        perror("Socket initialization error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(mPort);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    int bindResult = bind(mServerSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (bindResult == -1) {
        perror("Binding creation error");
        exit(EXIT_FAILURE);
    }

    std::cout << "Binding successful" << std::endl;
}

void Server::run() {
    if (mServerSocket == -1) {
        std::cout << "Server socket has never been initialized!\n"
                     "Init server socket, create binding and then try again!" << std::endl;
        return;
    }

    listen(mServerSocket, SOMAXCONN);

    while (true) {
        fd_set requestSockets, responseSockets;
        FD_ZERO(&requestSockets);
        FD_ZERO(&responseSockets);

        FD_SET(mServerSocket, &requestSockets);
        FD_SET(mServerSocket, &responseSockets);

        int maxSocketDescr = mServerSocket;

        for (auto connectedSocket: mConnectedSockets) {
            FD_SET(connectedSocket, &requestSockets);
            FD_SET(connectedSocket, &responseSockets);

            maxSocketDescr = std::max(maxSocketDescr, connectedSocket);
        }

        select(maxSocketDescr + 1, &requestSockets, &responseSockets, nullptr, nullptr);

        char buffer[BUFFER_SIZE];

        for (auto requestSocket: mConnectedSockets) {
            if (FD_ISSET(requestSocket, &requestSockets)) {
                memset(buffer, 0, BUFFER_SIZE);

                int receiveResult = recv(requestSocket, buffer, BUFFER_SIZE, MSG_NOSIGNAL);

                if (receiveResult <= 0) {
                    mConnectedSockets.erase(requestSocket);
                    shutdown(requestSocket, SHUT_RDWR);
                    close(requestSocket);
                    std::cout << "MINUS " << requestSocket << std::endl;
                    continue;
                }

                for (auto responseSocket: mConnectedSockets) {
                    if (FD_ISSET(responseSocket, &responseSockets) && responseSocket != requestSocket) {
                        send(responseSocket, buffer, BUFFER_SIZE, MSG_NOSIGNAL);
                    }
                }
            }
        }

        if (FD_ISSET(mServerSocket, &requestSockets)) {
            int receivedSocket = accept(mServerSocket, nullptr, nullptr);
            set_nonblock(receivedSocket);
            mConnectedSockets.insert(receivedSocket);
            std::cout << "PLUS " << receivedSocket << std::endl;
        }

    }
}