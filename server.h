//
// Created by roman on 20.05.19.
//

#ifndef OS_NET_SERVER_H
#define OS_NET_SERVER_H


#include <array>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include "my_fd.h"

static constexpr char response_prefix[] = "Hello, ";
static constexpr size_t MAX_EVENTS = 32;

class server
{
public:
    server();

    void start(const char*, const in_port_t);
private:
    static constexpr size_t BUFFER_SIZE = 4096;
    my_fd tcp_socket;
    char buffer[BUFFER_SIZE]; //for null terminated string
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];

};


#endif //OS_NET_SERVER_H
