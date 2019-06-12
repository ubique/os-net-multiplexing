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


    epoll_event.data.fd = STDIN_FILENO;
    check_error(epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, STDIN_FILENO, &epoll_event), "epoll_ctl");

    char buffer[BUFFER_SIZE];
    while (true) {
        int descriptors_count = epoll_wait(epoll_descriptor, events, MAX_CONNECTS, -1);
        check_error(descriptors_count, "epoll_wait");

        for (int i = 0; i < descriptors_count; i++) {
            if (events[i].data.fd == file_descriptor) {
                memset(buffer, 0, BUFFER_SIZE);

                int message_len = 0;
                int current_recv;
                while ((current_recv = recv(file_descriptor, buffer + message_len, sizeof(buffer) - message_len, 0)) > 0) {
                    message_len += current_recv;
                    if (buffer[message_len - 1] == '\n') {
                        break;
                    }
                }
                check_error(current_recv, "recv");
                buffer[message_len] = '\0';

                if (message_len == 0) {
                    cout << "Server closed" << endl;
                    check_error(close(file_descriptor), "close");
                    return 0;
                }

                cout << "answer from the server: " << buffer;
            } else if (events[i].data.fd == 0) {
                string message;
                cin >> message;
                if (message == ":q") {
                    check_error(close(file_descriptor), "close");
                    return 0;
                } else {
                    message += '\n';

                    int total_send = 0;
                    while (total_send < message.size()) {
                        int status = send(file_descriptor, message.data() + total_send, message.size() - total_send, 0);
                        check_error(status, "send");
                        total_send += status;
                    }
                }
            }
        }
    }
}