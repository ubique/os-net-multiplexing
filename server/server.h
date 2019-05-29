//
// Created by Anton Shelepov on 2019-05-17.
//

#ifndef OS_NET_SERVER_H
#define OS_NET_SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <exception>
#include <string>
#include "../socket_descriptor/socket_descriptor.h"
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>

class server {

public:
    server(std::string const& address, int port);
    void log(std::string const& msg);
    void run();

private:
    void add_to_epoll(int sd, uint32_t events);
    void remove_from_epoll(int sd);
    std::string read(int desc);
    void send(int desc, std::string const& message);

    socket_descriptor socket_fd = -1;
    socket_descriptor epoll_fd = -1;
    sockaddr_in addr = {0};

    static const int BACKLOG_QUEUE_SIZE = 50;
    static const int MAX_EVENTS = 100;
    static const size_t BUFFER_SIZE = 10 * 4096;

    epoll_event events[MAX_EVENTS];

    chat_engine engine;

    std::unordered_map<int, socket_descriptor> sockets;
};


struct server_exception : std::runtime_error {
    server_exception(std::string const& msg);
};


#endif //OS_NET_SERVER_H