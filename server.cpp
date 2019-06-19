#include <iostream>

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
    int serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (serverSocket == -1) {
        perror("Create socket error");
        exit(EXIT_FAILURE);
    }

    int epoll = epoll_create1(0);
    if (epoll == -1) {
        perror("Create epoll error");
        exit(EXIT_FAILURE);
    }

    struct epoll_event curClient, clients[MAX_EVENTS];
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(SERVER_PORT);
    address.sin_addr.s_addr = INADDR_ANY;
    if (bind(serverSocket, (sockaddr *) &address, sizeof(address)) == -1) {
        perror("Bind socket error");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 5) == -1) {
        perror("Listening error");
        exit(EXIT_FAILURE);
    }

    curClient.events = EPOLLIN;
    curClient.data.fd = serverSocket;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, serverSocket, &curClient) == -1) {
        perror("Add to epoll error");
        closeSockets(epoll, serverSocket);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is ready..." << std::endl;
    while (true) {
        int clientsAmount = epoll_wait(epoll, clients, MAX_EVENTS, -1);
        if (clientsAmount == -1) {
            perror("Epoll wait error");
            closeSockets(epoll, serverSocket);
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < clientsAmount; ++i) {
            if (clients[i].data.fd == serverSocket) {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);

                int clientSocket = accept(serverSocket, (sockaddr *) &client, &len);
                if (clientSocket == -1) {
                    perror("Connect client socket error");
                    continue;
                }
                std::cout << "Client \'" << clientSocket << "\' connected!" << std::endl;

                curClient.events = EPOLLIN;
                curClient.data.fd = clientSocket;
                if (epoll_ctl(epoll, EPOLL_CTL_ADD, clientSocket, &curClient) == -1) {
                    perror("Add to epoll error");
                    closeSockets(epoll, serverSocket);
                    exit(EXIT_FAILURE);
                }
            } else {
                char request[MAX_BUFFER_SIZE] = {};
                int receiveResult = recv(clients[i].data.fd, request, MAX_BUFFER_SIZE, 0);
                if (receiveResult <= 0) {
                    perror("Receive error");

                    if (epoll_ctl(epoll, EPOLL_CTL_DEL, clients[i].data.fd, nullptr) == -1) {
                        perror("Delete from epoll error");
                    }

                    if (close(clients[i].data.fd) == -1) {
                        perror("Close socket error");
                    }

                    break;
                }

                int responseSize = 0, sendTriesCount = 3;
                while (responseSize < receiveResult) {
                    int sentPart = send(clients[i].data.fd, request + responseSize, receiveResult - responseSize, 0);
                    if (sentPart == -1) {
                        perror("Send error");
                        --sendTriesCount;
                    }
                    responseSize += sentPart;
                    if (sendTriesCount == 0) {
                        std::cout << "Message sending error" << std::endl;

                        if (epoll_ctl(epoll, EPOLL_CTL_DEL, clients[i].data.fd, nullptr) == -1) {
                            perror("Delete from epoll error");
                        }

                        if (close(clients[i].data.fd) == -1) {
                            perror("Close socket error");
                        }

                        break;
                    }
                }
            }
        }
    }

    closeSockets(epoll, serverSocket);
    exit(EXIT_SUCCESS);
}
