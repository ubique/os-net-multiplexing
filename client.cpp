#include <iostream>
#include <cstring>

#include <unistd.h>

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define MAX_BUFFER_SIZE 1024
#define SERVER_PORT 8080
#define MAX_EVENTS 32

void closeSockets(int epoll, int mySocket) {
    if (close(epoll) == -1) {
        perror("Close epoll error");
        exit(EXIT_FAILURE);
    }

    if (close(mySocket) == -1) {
        perror("Close socket error");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == -1) {
        perror("Create socket error");
        exit(EXIT_FAILURE);
    }

    int epoll = epoll_create1(0);
    if (epoll == -1) {
        perror("Create epoll error");

        if (close(clientSocket) == -1) {
            perror("Close socket error");
        }

        exit(EXIT_FAILURE);
    }

    struct epoll_event curClient, clients[MAX_EVENTS];
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(SERVER_PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(clientSocket, (sockaddr *) &address, sizeof(address)) == -1) {
        perror("Connect socket error");
        closeSockets(epoll, clientSocket);
        exit(EXIT_FAILURE);
    }

    curClient.events = EPOLLIN;
    curClient.data.fd = clientSocket;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, clientSocket, &curClient) == -1) {
        perror("Add to epoll error");
        closeSockets(epoll, clientSocket);
        exit(EXIT_FAILURE);
    }

    curClient.data.fd = 0;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, 0, &curClient) == -1) {
        perror("Add to epoll error");
        closeSockets(epoll, clientSocket);
        exit(EXIT_FAILURE);
    }

    char response[MAX_BUFFER_SIZE] = {};
    int messageSize = 0;

    std::cout << "Connected! Enter messages to server!" << std::endl;
    while (true) {
        if (feof(stdin)) {
            std::cout << std::endl;
            closeSockets(epoll, clientSocket);
            exit(EXIT_FAILURE);
        }

        int clientsCount = epoll_wait(epoll, clients, MAX_EVENTS, -1);
        if (clientsCount == -1) {
            perror("Epoll waiting error");
            closeSockets(epoll, clientSocket);
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < clientsCount; ++i) {
            if (clients[i].data.fd == clientSocket) {
                int messageLength = 0;
                std::cout << "Response is: ";

                while (messageLength < messageSize) {
                    int receivedPart = recv(clientSocket, response, MAX_BUFFER_SIZE, 0);
                    if (receivedPart == -1 || receivedPart == 0) {
                        perror("Receive error");
                        closeSockets(epoll, clientSocket);
                        exit(EXIT_FAILURE);
                    }

                    messageLength += receivedPart;
                    for (size_t j = 0; j < receivedPart; ++j) {
                        std::cout << response[j];
                    }
                }

                std::cout << std::endl;
            } else {
                std::cin >> response;
                messageSize = strlen(response);
                int fullMessageSize = 0;

                while (fullMessageSize < messageSize) {
                    int sentPart = send(clientSocket, response + fullMessageSize, messageSize - fullMessageSize, 0);
                    if (sentPart == -1) {
                        perror("Sent error");
                        closeSockets(epoll, clientSocket);
                        exit(EXIT_FAILURE);
                    }

                    fullMessageSize += sentPart;
                }
            }
        }
    }

    closeSockets(epoll, clientSocket);
    exit(EXIT_SUCCESS);
}