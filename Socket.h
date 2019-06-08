//
// Created by daniil on 27.05.19.
//

#ifndef OS_NET_MULTIPLEXING_SOCKET_H
#define OS_NET_MULTIPLEXING_SOCKET_H


#include <netinet/in.h>
#include <string>
#include <cstring>
#include <stdexcept>

struct socket_exception : std::runtime_error {
    socket_exception(std::string const& cause)
            : std::runtime_error(cause + ": " + strerror(errno)) {}
};


class Socket {
    int fd;
    int flags;
    static const int MAX = 500;
    static const int BUFFER_SIZE = 2048;
public:
    explicit Socket(int fd);
    Socket();
    Socket(Socket const &) = delete;
    Socket(Socket&&) noexcept;

    Socket& operator=(Socket const&) = delete;
    Socket&operator=(Socket&&) noexcept;

    void bind(sockaddr_in& address);
    void listen(int connections = MAX);
    Socket accept();

    int connect(sockaddr_in& address);

    size_t send(std::string& data);
    std::string read();

    int getDescriptor();

    void unblock();

    void close();

};


#endif //OS_NET_MULTIPLEXING_SOCKET_H
