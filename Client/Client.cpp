//
// Created by Noname Untitled on 26.05.19.
//

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstring>

#include <unistd.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Client.h"

Client::Client(char *name, char *address, char *port) : mName(name), mClientSocket(-1), mAddress(address) {
    mPort = std::stoi(port);

    if (mPort < MIN_PORT || mPort > MAX_PORT) {
        std::ostringstream errorMsg;
        errorMsg << "Port should be numeric value from " << MIN_PORT << " to " << MAX_PORT << "!\n";
        throw std::invalid_argument(errorMsg.str());
    }
}

Client::~Client() {
    if (mClientSocket != -1) {
        shutdown(mClientSocket, SHUT_RDWR);
        close(mClientSocket);
    }

    std::cout << "Terminate client connection!" << std::endl;
}

void Client::createConnection() {
    mClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (mClientSocket == -1) {
        perror("Socket initialization error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(mPort);
    inet_pton(AF_INET, mAddress, &(serverAddress.sin_addr));

    int connectionResult = connect(mClientSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (connectionResult == -1) {
        perror("Connection error");
        exit(EXIT_FAILURE);
    }

    std::cout << "Connection successful!" << std::endl;
}

void Client::run() {
    if (mClientSocket == -1) {
        std::cout << "Client socket has never been initialized!\n"
                     "Init client socket, create connection and then try again!" << std::endl;
        return;
    }

    std::cout << "Client is running..." << std::endl;

    char buffer[BUFFER_SIZE];

    while (true) {

        int fd = 0;
        fd_set requestToServer, responseFromServer;
        FD_ZERO(&requestToServer);
        FD_ZERO(&responseFromServer);

        FD_SET(mClientSocket, &requestToServer);
        FD_SET(mClientSocket, &responseFromServer);
        FD_SET(fd, &requestToServer);
        FD_SET(fd, &responseFromServer);

        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        int selectResult = select(8, &responseFromServer, nullptr, nullptr, &timeout);

        if (selectResult != 0) {
            if (FD_ISSET(0, &responseFromServer)) {
                memset(buffer, 0, 1024);
                int readResult = read(fd, buffer, 1024);

                if (readResult > 0) {
                    std::string message;
                    message.append(mName);
                    message.append(": ");
                    message.append(buffer);

                    send(mClientSocket, message.c_str(), message.size(), MSG_NOSIGNAL);
                }
            }

            if (FD_ISSET(mClientSocket, &responseFromServer)) {
                memset(buffer, 0, BUFFER_SIZE);
                int readResult = read(mClientSocket, buffer, BUFFER_SIZE);

                if (readResult <= 0) {
                    shutdown(mClientSocket, SHUT_RDWR);
                    close(mClientSocket);
                    return;
                } else {
                    std::cout << buffer;
                }
            }
        }
    }

}

