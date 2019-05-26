//
// Created by Noname Untitled on 22.05.19.
//

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstring>

#include <unistd.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Client.h"

void showUsage(char *fileName) {
    std::cerr << "Usage: " << fileName << " <Name> <Address> <Port>." << std::endl;
}

int main(int argc, char **argv) {

    if (argc != 4) {
        std::cerr << "Wrong usage!" << std::endl;
        showUsage(argv[0]);
        exit(EXIT_FAILURE);
    }

    Client myClient = Client(argv[1], argv[2], argv[3]);

    myClient.createConnection();
    myClient.run();

    exit(EXIT_SUCCESS);
}