#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <map>
#include "Message.h"
#include "Session.h"

class POP3Server {
public:
    int run();
    POP3Server(const std::string& host_name, int port);
    ~POP3Server();
private:
    const int COUNT_OF_POOLS = 16;
    const int COUNT_OF_EVENTS = 8;
    int socket_fd{};
    struct sockaddr_in server_addr{};

    void print_error(const std::string &msg);
    size_t get_size_of_vector(std::vector<Message> & messages);
    int epoll_ctl_wrap(int epool_fd, int op, int fd, uint32_t events);
    int delete_socket(int epoll, int fd, std::map<int, Session> &sessions);
    void stop();
};