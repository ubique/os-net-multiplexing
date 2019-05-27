//
// Created by roman on 20.05.19.
//

#ifndef OS_NET_SERVER_H
#define OS_NET_SERVER_H


#include <array>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
static constexpr char response_prefix[] = "Hello, ";
static constexpr size_t MAX_EVENTS = 32;

class hello_server
{
public:
    hello_server();

    void start(const char*, const in_port_t);
private:
    void make_response();
private:
    static constexpr size_t BUFFER_SIZE = 4096;
    int tcp_socket;
    char buffer[BUFFER_SIZE + sizeof(response_prefix) + 1]; //for null terminated string
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];

};


#endif //OS_NET_SERVER_H
