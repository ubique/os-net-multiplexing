#include "epolling.h"

namespace epoll {

void add(int const& epoll_fd, int const& fdesc, uint32_t const& events) {
    epoll_event event;
    event.events = events;
    event.data.fd = fdesc;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fdesc, &event) == -1) {
        throw any_net_exception("Cannot add descriptor to epoll");
    }
}

void remove(int const& epoll_fd, int const& fdesc) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fdesc, nullptr) == -1) {
        throw any_net_exception("Cannot remove descriptor from epoll");
    }
}

void change_mode(int const& epoll_fd, int const& fd, uint32_t const& events) {
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
        throw any_net_exception("Cannot change epolls mode");
    }
}
} // namespace epoll

std::string next(std::string& data) {
    size_t end = data.find("\r\n");
    if (end == std::string::npos) {
        return "";
    }
    std::string res = data.substr(0, end);
    data = data.substr(end + 2);
    return res;
}
