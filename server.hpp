#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

#include <string>

class server {
public:
    server(const std::string &address, const std::string &port);

    void wait_clients();

private:
    void check_ipv4(const std::string &address);
    void create_socket();
    void set_socket_options(const std::string &port);
    void bind_to_address();
    void accept_connection();

    void detach(int fd);

    void print_fatal_error(const std::string &err);

    //    std::optional<descriptor> server_fd;
    //    std::optional<descriptor> new_socket;
    int server_fd;
    int epoll_fd;
    struct sockaddr_in server_address;
    int addrlen = sizeof(server_address);

    const size_t max_events = 50;
};

#endif  // SERVER_H
