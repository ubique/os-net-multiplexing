#ifndef UTILS_H
#define UTILS_H 

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <string>

namespace utils {
    const size_t BUF_SIZE = 256;
    const size_t BACKLOG = 10;

    void epoll_create(int& epoll);
    void init(int& sockfd, sockaddr_in& server, const std::string& addr, const std::string& port, int type);
    void bind_and_listen(int& sockfd, sockaddr_in& server);
    void connect(int& sockfd, sockaddr_in& server);
    bool accept(int& resultfd, int sockfd, sockaddr* addr, socklen_t* len);

    bool send(int sockfd, char* message, ssize_t size);
    bool recv(int sockfd, char* message, ssize_t size);

    void close(int sockfd);

    void abort(int epoll, int sockfd);
    bool add(int epoll, int sockfd, epoll_event* ev);
}

#endif
