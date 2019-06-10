//
// Created by Yaroslav on 08/06/2019.
//

#include "utils.h"

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>

using std::cout;
using std::string;


void fun_send(char *what, int amount, int where, string message) {
    int counter = 0;
    while (counter < amount) {
        int sent = send(where, what + counter, amount - counter, MSG_NOSIGNAL);
//        int sent = send(where, what + counter, amount - counter, SO_NOSIGPIPE);
        if (sent == -1) {
            cout << "error occurred with sent in " << message << "\n";
            cout << strerror(errno) << "\n";
            exit(0);
        }
        counter += sent;
    }
}

void fun_recv(char *where, int amount, int from, string message) {
    int counter = 0;
    while(counter < amount) {
        int received = recv(from, where + counter, amount - counter, MSG_NOSIGNAL);
//        int received = recv(from, where + counter, amount - counter, SO_NOSIGPIPE);
        if (received == -1) {
            cout << "error occurred with recv "<< message << "\n";
            cout << strerror(errno) << "\n";
            exit(0);
        }
        counter += received;
    }
}

void add_new_client(const int* master, sockaddr_in* client, int* poll_s, socklen_t* size_s) {
    int s = accept(*master, (sockaddr *) (&client), size_s);
    is_failure(s, "accept");
    struct epoll_event epoll_event;
    epoll_event.data.fd = s;
    epoll_event.events = EPOLLIN;
    is_failure(epoll_ctl(*poll_s, EPOLL_CTL_ADD, s, &epoll_event), "epoll_ctl");
}

void is_failure(int rc, const string &problem_name) {
    if (rc == -1) {
        cout << problem_name << "\n";
        cout << "error occurred with " << strerror(errno) << "\n";
        exit(EXIT_FAILURE);
    }
}