#ifndef EPOLL_UTILS
#define EPOLL_UTILS

#include <stdexcept>
#include <string>

#include <cstring>
#include <sys/epoll.h>

struct any_net_exception : std::runtime_error {
    any_net_exception(std::string cause)
        : std::runtime_error(cause + ": " + strerror(errno)) {}
};

struct server_exception : any_net_exception {
    server_exception(std::string const& cause) : any_net_exception(cause) {}
};

struct client_exception : any_net_exception {
    client_exception(std::string cause) : any_net_exception(cause) {}
};

namespace epoll {
void add(int const& epoll_fd, int const& fdesc, uint32_t const& events);

void remove(int const& epoll_fd, int const& fdesc);

void change_mode(int const& epoll_fd, int const& fd, uint32_t const& events);
} // namespace epoll

std::string next(std::string& data);

#endif // EPOLL_UTILS
