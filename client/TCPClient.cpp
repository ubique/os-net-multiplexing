//
// Created by ifkbhit on 07.05.19.
//

#include "TCPClient.h"

net::TCPClient::TCPClient(const std::string& ipv4, const std::string& port) : TCPSocket(ipv4,
                                                                                        port) {}


void net::TCPClient::run(const data_t& data) {

    epoll_evt readEvt, writeEvt;

    readEvt.data.fd = getFileDescriptor();
    writeEvt.data.fd = getFileDescriptor();

    writeEvt.events = EPOLLOUT;
    readEvt.events = EPOLLIN;

    epollCtl(EPOLL_CTL_ADD, &writeEvt);

    while (true) {
        std::string input;
        std::cin >> input;
        if (input == "exit") {
            break;
        }
        epollModify(&writeEvt);
        int available = epoll_wait(epoll, evts, MAX_EVENTS, -1);
        if (available < 0) {
            Utils::assertPerror("Couldn't wait epoll");
        }

        for (int i = 0; i < getDescNumber(); i++) {
            if (evts[i].data.fd == getFileDescriptor()) {
                if (send(getFileDescriptor(), input.data(), input.size(), 0) == -1) {
                    close(getFileDescriptor());
                    Utils::assertPerror("client send");
                }
                Utils::message("'" + input + "' was sent");
            }
        }
        epollModify(&readEvt);
        for (int i = 0; i < getDescNumber(); i++) {
            if (evts[i].data.fd == getFileDescriptor()) {
                data_t recData;
                Utils::assertTrue(read(getFileDescriptor(), recData, input.size()) == -1, "");
            }
        }


    }
}

bool net::TCPClient::afterCreate(const sockaddr_in& in) {
    if (connect(getFileDescriptor(), (struct sockaddr*) &in, sizeof(in)) < 0 && errno != EINPROGRESS) {
        return Utils::fail("Couldn't connect to " + fullAddress());
    }
    Utils::message("Connected to " + fullAddress());
    return true;
}
