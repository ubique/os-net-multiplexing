//
// Created by Noname Untitled on 22.05.19.
//

#include <iostream>
#include <sstream>
#include <set>

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "Server.h"

void showUsage(char *fileName) {
    std::cerr << "Usage: " << fileName << " <Port>." << std::endl;
}

int main(int argc, char **argv) {

    if (argc != 2) {
        std::cerr << "Wrong usage!" << std::endl;
        showUsage(argv[0]);
        exit(EXIT_FAILURE);
    }

    Server myServer = Server(argv[1]);

    myServer.createBinding();
    myServer.run();

    exit(EXIT_SUCCESS);
}
