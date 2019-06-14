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
#include <unistd.h>

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



    epoll_event.events = STDIN_FILENO;


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
                } else {
                    message += "\0";

                    int sent = 0;
                    while (sent < message.size()){
                        int new_sent;
                        if ((new_sent = (send(fileDes, message.data(), message.size(), 0)) == -1)){
                            cerr << "Can't send" << endl;
                            exit(EXIT_FAILURE);
                        }
                        sent += new_sent;
                    }
                }
            } else if (cur == fileDes) {
                for (int i = 0; i < BUFFER_SIZE; i++){
                    buff[i] = 0;
                }

                int len = 0;
                int new_len = 1;
                while(new_len > 0) {
                    new_len = recv(fileDes, buff + len, BUFFER_SIZE - len, 0);
                    if (new_len == -1) {
                        cerr << "Can't read response" << endl;
                        continue;
                    }
                    len += new_len;
                    if (buff[len - 1] == '\n'){
                        break;
                    }
                }
                if (new_len == -1) {
                    cerr << "Can't read response" << endl;
                    continue;
                }
                if (len == 0){
                    return 0;
                }
                else {
                    buff[len] = '\0';
                    cout << buff << endl;
                }
            }
        }
    }
    return 0;
}