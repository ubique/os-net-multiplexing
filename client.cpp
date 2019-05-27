#include <iostream>
#include <sys/socket.h>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <stdio.h>
#include <sys/epoll.h>
#include <cstdlib>


using namespace std;

int const CONNECTION_COUNT = 32;
int const BUFFER_SIZE = 8192;

int main(int argc, char **argv) {
    if (argc != 3) {
        cerr << "Usage: ./client host port" << endl;
        exit(EXIT_FAILURE);
    }

    int port;
    try {
        port = htons(stoi(argv[2]));
    }
    catch (...) {
        cerr << "Invalid port: " << port << endl;;
    }

    int fileDes = socket(AF_INET, SOCK_STREAM, 0);
    if (fileDes == -1) {
        cerr << "Can't create socket" << endl;;
        exit(EXIT_FAILURE);
    }

    int epollDes = epoll_create1(0);

    struct sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = port;

    if (inet_pton(AF_INET, argv[1], &server.sin_addr) != 1) {
        cerr << ("Wrong adress") << endl;;
        exit(EXIT_FAILURE);
    }

    if (connect(fileDes, (sockaddr * )(&server), sizeof(server)) == -1) {
        cerr << ("Connection failed") << endl;;
        exit(EXIT_FAILURE);
    }


    struct epoll_event epoll_event{};
    struct epoll_event events[CONNECTION_COUNT];


    epoll_event.events = EPOLLIN;
    epoll_event.data.fd = fileDes;

    epoll_event.events = EPOLLIN;
    epoll_event.data.fd = 0;


    char buff[BUFFER_SIZE];
    bool working = true;
    while (working) {
        int ready_cnt = epoll_wait(epollDes, events, CONNECTION_COUNT, -1);
        if (ready_cnt == -1){
            cerr << "Can't wait epoll" << endl;
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < ready_cnt; i++) {
            int cur = events[i].data.fd;
            if (cur == 0) {
                cout << "Enter a message:" << endl;
                string message;
                cin >> message;
                if (message == "exit") {
                    working = false;
                } else if (send(fileDes, message.data(), message.size(), 0) == -1){
                    cerr << "Sending failed" << endl;
                }
            } else if (cur == fileDes) {
                for (int i = 0; i < BUFFER_SIZE; i++){
                    buff[i] = 0;
                }
                int len = recv(fileDes, buff, BUFFER_SIZE, 0);
                if (len == -1){
                    cerr << "Can't read response" << endl;
                    continue;
                }
                cout << buff << endl;
            }
        }
    }
    return 0;
}