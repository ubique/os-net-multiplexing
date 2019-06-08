//
// Created by daniil on 27.05.19.
//

#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <vector>
#include "Epoll.h"
#include "Client.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Wrong usage: two arguments excepted" << std::endl;
        return 0;
    }
    try {
        Client client(argv[1], static_cast<uint16_t >(std::stoul(argv[2])));
        client.run();
    } catch (client_error& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}