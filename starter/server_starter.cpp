//
// Created by Михаил Терентьев on 2019-05-23.
//

#include <iostream>
#include "../server-lib/server_execption.h"
#include "../server-lib/server.cpp"
int main(int argc, char *argv[])
{
    std::string address = argc < 2 ? "127.0.0.1" : std::string{argv[1]};
    std::string port = argc < 3 ? "8888" : std::string{argv[2]};
    try {
        server s(address, port);
        s.run();
    } catch (const std::exception &e) {
        std::cerr << e.what();
        if (errno != 0) {
            std::cerr << ": " << strerror(errno);
        }
        std::cerr << std::endl;
    }
}