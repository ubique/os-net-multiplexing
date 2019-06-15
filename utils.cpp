//
// Created by domonion on 02.05.19.
//

#include "utils.hpp"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

using std::cout;
using std::endl;
using std::string;

void check_error(int rc, const string &additional) {
    if (rc == -1) {
        int error = errno;
        cout << additional << endl;
        cout << strerror(error) << endl;
        exit(0);
    }
}

void doSend(char *what, int amount, int where) {
    int counter = 0;
    while (counter < amount) {
        int sent = write(where, what + counter, amount - counter);
        if (sent == -1 && errno != EAGAIN)
            check_error(sent, "send");
        counter += std::max(0, sent);
    }
}

void doRecv(char *where, int amount, int from) {
    int counter = 0;
    while (counter < amount) {
        int received = read(from, where + counter, amount - counter);
        if (received == -1 && errno != EAGAIN)
            check_error(received, "recv");
        counter += std::max(0, received);
    }
}
