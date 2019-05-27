#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <zconf.h>

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
        cout << "Usage: ./server  host port" << endl;

    }


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
    check_error((bind(file_descriptor, (sockaddr *) (&server), sizeof(server))), "bind");
    check_error(listen(file_descriptor, SOMAXCONN), "listen");


    int epoll_descriptor = epoll_create1(0);
    struct epoll_event epoll_event{};
    struct epoll_event events[MAX_CONNECTS];

    epoll_event.events = EPOLLIN;
    epoll_event.data.fd = 0;
    check_error(epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, 0, &epoll_event), "epoll_ctl");

    epoll_event.events = EPOLLIN;
    epoll_event.data.fd = file_descriptor;
    check_error(epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, file_descriptor, &epoll_event), "epoll_ctl");


    char buffer[BUFFER_SIZE];
    while (true) {
        int descriptors_count = epoll_wait(epoll_descriptor, events, MAX_CONNECTS, -1);
        check_error(descriptors_count, "epoll_wait");

        for (int i = 0; i < descriptors_count; i++) {
            int descriptor = events[i].data.fd;
            if (descriptor == file_descriptor) {
                struct sockaddr_in client_addr{};
                socklen_t address_size = sizeof(client_addr);

                int accept_descriptor = accept(file_descriptor, (struct sockaddr*) &client_addr, &address_size);
                if (accept_descriptor == -1) {
                    perror("accept");
                    continue;
                }

                cout << "new client" << endl;
                int flags = fcntl(accept_descriptor, F_GETFL, 0);
                fcntl(accept_descriptor, F_SETFL, flags | O_NONBLOCK);

                epoll_event.events = EPOLLIN | EPOLLET;
                epoll_event.data.fd = accept_descriptor;
                check_error(epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, accept_descriptor, &epoll_event), "epoll_ctl");
            } else {
                memset(buffer, 0, BUFFER_SIZE);

                int message_len = recv(descriptor, &buffer, sizeof(buffer), 0);
                if (message_len == -1) {
                    check_error(epoll_ctl(epoll_descriptor, EPOLL_CTL_DEL, descriptor, NULL), "epoll_ctl");
                } else {
                    if (message_len == 0) {
                        cout << "close client" << endl;
                    } else {
                        cout << "message from client= " << descriptor << ": " << buffer << endl;
                    }
                    if (send(descriptor, buffer, message_len, 0) == -1) {
                        perror("send");
                        check_error(epoll_ctl(epoll_descriptor, EPOLL_CTL_DEL, descriptor, NULL), "epoll_ctl");
                    }
                }
            }
        }
    }



}