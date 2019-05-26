#ifndef MY_SERVER
#define MY_SERVER

#include <stdexcept>
#include <unordered_map>

#include <arpa/inet.h>
#include <sys/epoll.h>

#include "epolling.h"
#include "socket/socket_wrapper.h"

struct server {
    server(char* address, uint16_t port);

    server(server const&) = delete;
    server& operator=(server const&) = delete;

    [[noreturn]] void run();

    ~server();

  private:
    static constexpr int MAX_EVENTS = 200;
    static constexpr int MAX_CONN = 1000;

    sockaddr_in server_address;
    int server_desc;

    epoll_event* events;
    int epoll_fd;
    std::unordered_map<int, ssocket> sockets;

    std::unordered_map<int, std::string> recieved, sending;
};

#endif // MY_SERVER
