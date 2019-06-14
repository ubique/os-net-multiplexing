//
// Created by Yaroslav on 08/06/2019.
//

#include "utils.h"

void check_error(int rc, const string &additional) {
    if (rc < 0) {
        int error = errno;
        cout << additional << "\n";
        cout << strerror(error) << "\n";
        exit(0);
    }
}

void check_non_stop(int rc, const std::string& additional) {
    if (rc < 0) {
        int error = errno;
        cout << additional << "\n";
        cout << strerror(error) << "\n";
    }
}

void fun_send(int fd, char* what, int size, string message) {
    int counter = 1;
    int sent_sum = 0;
    while (sent_sum != size && counter > 0) {
        int counter = send(fd, what + sent_sum, size - sent_sum, MSG_NOSIGNAL);
//        int sent = send(where, what + counter, amount - counter, SO_NOSIGPIPE);
        if (counter == -1) {
            cout << "error occurred with sent in " << message << "\n";
            cout << strerror(errno) << "\n";
            exit(0);
        }
        sent_sum += counter;
    }
}

bool fun_send_client(int fd, char* what, int size, string message) {
    int counter = 1;
    int sent_sum = 0;
    while (sent_sum != size && counter > 0) {
        int counter = send(fd, what + sent_sum, size - sent_sum, MSG_NOSIGNAL);
//        int sent = send(where, what + counter, amount - counter, SO_NOSIGPIPE);
        sent_sum += counter;
    }
    if (counter < 0) {
        return true;
    }
    return false;
}

// fix it!!!!
void fun_recv(int fd, char* what, string message) {
    int counter = 1;
    int sent_sum = 0;
    while((counter = recv(fd, what + sent_sum, BUF_SIZE - sent_sum, MSG_NOSIGNAL)) > 0) {
//        int received = recv(from, where + counter, amount - counter, SO_NOSIGPIPE);
        if (counter == -1) {
            cout << "error occurred with recv "<< message << "\n";
            cout << strerror(errno) << "\n";
            exit(0);
        }
        if (*(what + sent_sum - 1) == '\n') {
            break;
        }
        sent_sum += counter;
    }
}

bool fun_recv_client(int fd, char* what, string message) {
    int counter = 1;
    int sent_sum = 0;
    while((counter = recv(fd, what + sent_sum, BUF_SIZE - sent_sum, MSG_NOSIGNAL)) > 0) {
//        int received = recv(from, where + counter, amount - counter, SO_NOSIGPIPE);
        if (*(what + sent_sum - 1) == '\n') {
            break;
        }
        sent_sum += counter;
    }
    if (counter < 0) {
        return true;
    }
    return false;
}


