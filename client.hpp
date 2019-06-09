#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>

#include <string>

class client {
public:
    client(const std::string &address, const std::string &port);

    void connect_to_server();
    void work();

private:
    void create_socket();
    void set_sock_options(const std::string &addr_to_connect,
                          const std::string &port);

    void print_fatal_error(const std::string &err);

    int client_fd;
    int epoll_fd;
    struct sockaddr_in serv_addr;

    const size_t max_events = 50;
};

#endif  // CLIENT_H
