//
// Created by roman on 20.05.19.
//

#include <iostream>
#include "client.h"
#include <stdio.h>

using std::cin;
using std::cout;
using std::string;

int main(int argc, char *argv[]) {
    const char* server_address = "127.0.0.1";
    int port = 8080;
    if (argc == 3) {
        server_address = argv[1];
        port = std::atoi(argv[2]);
    }
    client cl;
    cl.start(server_address, port);
    return 0;
}