//
// Created by vitalya on 19.05.19.
//

#include "server.h"

#include <iostream>
#include <csignal>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Address and port arguments expected" << std::endl;
        return EXIT_FAILURE;
    }
    try {
        server server(argv[1], atoi(argv[2]));
        server.start();
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
