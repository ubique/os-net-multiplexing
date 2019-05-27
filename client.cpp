#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <stdio.h>
#include <zconf.h>
#include <sys/epoll.h>

using namespace std;


const int MAX_CONNECTS = 16;
const int BUFFER_SIZE = 1024;

void check_error(int value, const char *message) {
    if (value == -1) {
        perror(message);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "Usage: ./client host port" << endl;
        exit(EXIT_SUCCESS);
    }



    int epoll_descriptor = epoll_create1(0);
    int file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    check_error(file_descriptor, "socket");
    struct sockaddr_in server{};
    server.sin_family = AF_INET;
    int port;
    try {
        port = stoi(argv[2]);
    } catch (...) {
        cerr << "Invalid port";
        exit(EXIT_FAILURE);
    }
    server.sin_port = htons(port);
    inet_pton(AF_INET, argv[1], &server.sin_addr);
    check_error(connect(file_descriptor, (sockaddr *) (&server), sizeof(server)), "connect");

    struct epoll_event epoll_event{};
    struct epoll_event events[MAX_CONNECTS];

    epoll_event.events = EPOLLIN;
    epoll_event.data.fd = file_descriptor;
    check_error(epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, file_descriptor, &epoll_event), "epoll_ctl");

    epoll_event.events = EPOLLIN;
    epoll_event.data.fd = 0;
    check_error(epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, 0, &epoll_event), "epoll_ctl");


    char buffer[BUFFER_SIZE];
    while (true) {
        int descriptors_count = epoll_wait(epoll_descriptor, events, MAX_CONNECTS, -1);
        check_error(descriptors_count, "epoll_wait");

        for (int i = 0; i < descriptors_count; i++) {
            if (events[i].data.fd == file_descriptor) {
                memset(buffer, 0, BUFFER_SIZE);
                int message_len = recv(file_descriptor, buffer, BUFFER_SIZE, 0);
                check_error(message_len, "recv");
                cout << "answer from the server: " << buffer << endl;
            } else if (events[i].data.fd == 0) {
                cout << "Enter a message:";
                string message;
                cin >> message;
                cout << endl;
                if (message == ":q") {
                    check_error(close(file_descriptor), "close");
                    return 0;
                } else {
                    check_error(send(file_descriptor, message.data(), message.size(), 0), "send");
                }
            }
        }
    }
}