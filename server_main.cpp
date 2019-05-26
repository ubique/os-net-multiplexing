#include "server/server.h"
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Wrong usage. 2 arguments required: <address> <port>"
                  << std::endl;
        return -1;
    }
    try {
        server server(argv[1], static_cast<uint16_t>(stoul(argv[2])));
        server.run();
    } catch (any_net_exception& e) {
        std::cerr << "Error during running server " << e.what() << std::endl;
        return -1;
    }
}
