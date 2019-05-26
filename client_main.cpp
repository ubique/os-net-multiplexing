#include <iostream>

#include "client/client.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Wrong usage. Two arguments expected: <address> <port>"
                  << std::endl;
        return -1;
    }
    try {
        client client(argv[1], static_cast<uint16_t>(std::stoul(argv[2])));
        client.interact();
    } catch (any_net_exception& e) {
        std::cerr << e.what();
        return -1;
    }
}
