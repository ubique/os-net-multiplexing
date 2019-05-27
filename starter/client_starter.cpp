//
// Created by Михаил Терентьев on 2019-05-13.
//
#include <iostream>
#include "../client-lib/client.h"
#include "../client-lib/client_exception.h"


int main(int argc, char *argv[])
{
    std::string address = argc < 2 ? "127.0.0.1" : std::string{argv[1]};
    std::string port = argc < 3 ? "8888" : std::string{argv[2]};
    try {
        client c(address, port);
        c.run();
    } catch (const std::exception &e) {
        std::cerr << e.what();
        if (errno != 0) {
            std::cerr << ": " << strerror(errno);
        }
        std::cerr << std::endl;
    }
}
