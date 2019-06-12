//
// Created by roman on 20.05.19.
//

#ifndef OS_NET_CLIENT_H
#define OS_NET_CLIENT_H


#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include "my_fd.h"

using std::string;
static constexpr size_t MAX_EVENTS = 32;

class client {
public:
    client();

    void start(const char *hostAddress, const in_port_t port);
    ssize_t receive_response();
    void do_request(const string& msg);

private:
    my_fd tcp_socket;
    static constexpr size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE]; //for null terminated string
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];

};


#endif //OS_NET_CLIENT_H
