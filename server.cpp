#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>

using namespace std;


const int MAX_CONNECTS = 32;
const int BUFFER_SIZE = 8192;


int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "Usage: ./server host port" << endl;
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server{};
    server.sin_family = AF_INET;
    int port;
    try {
        port = htons(stoi(argv[2]));
    } catch (...) {
        cerr << "Invalid port: " << endl;
        exit(EXIT_FAILURE);
    }

    int fileDes;
    fileDes = socket(AF_INET, SOCK_STREAM, 0);

    if (fileDes == -1) {
        cerr << "Can't create socket" << endl;
        exit(EXIT_FAILURE);
    }

    server.sin_port = port;

    if (inet_pton(AF_INET, argv[1], &server.sin_addr) != 1) {
        cerr << "Invalid address";
        exit(EXIT_FAILURE);
    } else if (listen(fileDes, 10) == -1) {
        cerr << "Can't listen";
        exit(EXIT_FAILURE);
    } else if (bind(fileDes, (sockaddr * ) & server, sizeof(server)) == -1) {
        cerr << "Can't bind";
        exit(EXIT_FAILURE);
    }

    int epollDes;
    epollDes = epoll_create(1);

    if (epollDes == -1) {
        cerr << "Can't create epoll";
        exit(EXIT_FAILURE);
    }

    struct epoll_event epoll_event{};
    struct epoll_event events[MAX_CONNECTS];
    epoll_event.data.fd = fileDes;
    epoll_event.events = EPOLLIN;

    if (epoll_ctl(epollDes, EPOLL_CTL_ADD, fileDes, &epoll_event) < 0) {
        cerr << "Can't add descriptor to epoll";
        exit(EXIT_FAILURE);
    }

    char buff[BUFFER_SIZE];

    bool working = true;
    while (working) {
        int ready_cnt = epoll_wait(epollDes, events, MAX_CONNECTS, -1);
        if (ready_cnt == -1) {
            cerr << "Can't wait epoll" << endl;
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < ready_cnt; i++) {
            int cur = events[i].data.fd;
            if (cur != fileDes) {
                for (char &i : buff) {
                    i = 0;
                }

                int len = recv(cur, &buff, sizeof(buff), 0);

                if (len == -1) {
                    cerr << "Can't receive a message" << endl;
                    continue;
                } else if (len == 0){
                    continue;
                }

                string message(buff, buff + len);
                if (message == "exit") {
                    working = false;
                }

                cout << "Received message " << cur << ": " << buff << endl;
                if (send(cur, buff, len, 0) == -1) {
                    cerr << "Send failed" << endl;
                    if ((epoll_ctl(epollDes, EPOLL_CTL_DEL, cur, NULL) == -1))
                        cerr << "Can't delete";
                }

            } else {
                struct sockaddr_in client_addr{};
                socklen_t address_size = sizeof(client_addr);

                int client_fd = accept(fileDes, (struct sockaddr *) &client_addr, &address_size);
                if (client_fd == -1) {
                    cerr << "Can't accept";
                    continue;
                }

                cout << "New client connected" << endl;

                fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);

                epoll_event.events = EPOLLIN;
                epoll_event.data.fd = client_fd;
                if (epoll_ctl(epollDes, EPOLL_CTL_ADD, client_fd, &epoll_event) == -1) {
                    cerr << "Can't add descriptor to epoll";
                }
            }
        }
    }
}