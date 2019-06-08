//
// Created by daniil on 26.05.19.
//

#include <iostream>
#include "Server.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Wrong number of arguments, expected 3" << std::endl;
    }
    try {
        Server server(argv[1], static_cast<uint16_t >(std::stoul(argv[2])));
        server.run();
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}