//
// Created by ifkbhit on 07.05.19.
//

#include "TCPClient.h"

net::TCPClient::TCPClient(const std::string& ipv4, const std::string& port) : TCPSocket(ipv4,
                                                                                        port) {}


void net::TCPClient::run(const data_t& data) {
//    aProtocol->sender(getFileDescriptor(), data);
    while (true) {
        for (int i = 0; i < getDescNumber(true); i++) {
            auto evt = changing[i].data.fd;
            if (evt == STDINFD) {
                std::string input;
                std::getline(std::cin, input);
                if (input != "exit") {
                    int success = send(getFileDescriptor(), input.data(), input.size(), 0);
                    if (success == -1) {
                        perror("client send");
                        close(getFileDescriptor());
                        exit(EXIT_FAILURE);
                    }
                    Utils::message("'" + input + "' was sent");
                } else {
                    close(getFileDescriptor());
                    exit(EXIT_SUCCESS);
                }
            } else if (evt == getFileDescriptor()) {
                //read
                data_t recData;
                if (read(getFileDescriptor(), recData, -1) <= 0) {

                    close(getFileDescriptor());
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

bool net::TCPClient::afterCreate(const sockaddr_in& in) {
    if (connect(getFileDescriptor(), (struct sockaddr*) &in, sizeof(in)) < 0) {
        return Utils::fail("Couldn't connect to " + fullAddress());
    }
    Utils::message("Connected to " + fullAddress());
    return true;
}
