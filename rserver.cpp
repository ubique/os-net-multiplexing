#include <cstdlib>
#include <iostream>
#include <string>

#include "server.hpp"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Server usage: <address> <port>" << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        server serv({argv[1]}, {argv[2]});
        serv.wait_clients();
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
    }
}
