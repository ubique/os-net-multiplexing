//
// Created by roman on 20.05.19.
//

#ifndef OS_NET_CLIENT_H
#define OS_NET_CLIENT_H


#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>

using std::string;
static constexpr size_t MAX_EVENTS = 32;

class client {
public:
    client();

    void connect_to(const char *hostAddress, const in_port_t port);

    void start();

    void disconnect();

private:
    int tcp_socket;
    static constexpr size_t BUFFER_SIZE = 4096 * 2;
    char buffer[BUFFER_SIZE]; //for null terminated string
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];

};


#endif //OS_NET_CLIENT_H
