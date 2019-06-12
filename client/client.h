//
// Created by Anton Shelepov on 2019-05-17.
//

#ifndef OS_NET_CLIENT_H
#define OS_NET_CLIENT_H

#include "../socket_descriptor/socket_descriptor.h"
#include <string>
#include <exception>
#include <netdb.h>
#include <sys/epoll.h>
#include <stdexcept>

class client {
public:
    client();
    void establish_connection(std::string const& address, int port);
    void log(std::string const& msg);
    void run();

private:
    void add_to_epoll(int sd, uint32_t events);
    std::string read(int desc);
    void send(int desc, std::string const& message);

    socket_descriptor socket_fd = -1;
    sockaddr_in server_addr = {0};

    socket_descriptor epoll_fd;

    const size_t BUFFER_SIZE = 10 * 4096;
    const size_t TRIES_NUMBER = 10;
    static const int MAX_EVENTS = 5;

    epoll_event events[MAX_EVENTS];

    bool alive = true;
    bool connected = false;
};

struct client_exception : std::runtime_error {
    client_exception(std::string const& msg);
};


#endif //OS_NET_CLIENT_H