//
// Created by max on 07.05.19.
//

#include "Epoll.h"
#include "helper.h"
#include <map>

using Symb = Printer::Symbols;

std::map<std::string, int> names{
        std::pair<std::string, int>("Mod", EPOLL_CTL_MOD),
        std::pair<std::string, int>("Add", EPOLL_CTL_ADD),
        std::pair<std::string, int>("Del", EPOLL_CTL_DEL)
};

Epoll::Epoll() : fd(-1) {}

void Epoll::start() {
    Printer::print(std::cout, "Epoll::start", Symb::End);
    fd = epoll_create1(0);
    checker(fd, "Unable to create epoll");

}

void Epoll::do_event(int a_fd, uint32_t flags, const std::string &mode) {
    Printer::print(std::cout, "Epoll::", mode, Symb::End);
    struct epoll_event event{};
    event.events = flags;
    event.data.fd = a_fd;
    if (auto iter = names.find(mode); iter != names.end()) {
        int res = epoll_ctl(fd, iter->second, a_fd, (mode == "Del" ? nullptr : &event));
        checker(res, std::string("Unable to ").append(mode).append(" epoll_event"));
    } else {
        throw std::runtime_error("Unknown operation with epoll");
    }
}

int Epoll::wait(struct epoll_event *events) {
    Printer::print(std::cout, "Epoll::wait", Symb::End);
    int nfds = epoll_wait(fd, events, BUFFER_SIZE - 1, -1);
    checker(nfds, "Epoll wait failed");
    return nfds;
}

void Epoll::close() {
    Printer::print(std::cout, "Epoll::close", Symb::End);
    int ret = ::close(fd);
    checker(ret, "Unable to close");
}

Epoll::~Epoll() {
    try {
        close();
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Failed" << std::endl;
    }
}

