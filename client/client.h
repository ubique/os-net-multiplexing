#ifndef MY_CLIENT
#define MY_CLIENT

#include <stdexcept>

#include <arpa/inet.h>
#include <cstring>

#include "epolling.h"
#include "socket/socket_wrapper.h"

struct client {
    client(char* address, uint16_t port);

    void interact();

    ~client();

  private:
    static constexpr int MAX_CONN = 1000;
    static constexpr int MAX_EVENTS = 3;

    ssocket client_socket;
    sockaddr_in server_address;

    epoll_event* events = new epoll_event[MAX_EVENTS];

    int epoll_fd;

    bool connected, stop;
};

#endif
