#ifndef SOCKETWRAP
#define SOCKETWRAP

#include <iostream>
#include <stdexcept>
#include <vector>

#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

struct socket_exception : std::runtime_error {
    socket_exception(std::string const& cause)
        : std::runtime_error(cause + ": " + strerror(errno)) {}
};

struct ssocket {
    ssocket();
    ssocket(ssocket const&) = delete;
    ssocket(ssocket&& other) noexcept;

    ssocket& operator=(ssocket const&) = delete;
    ssocket& operator=(ssocket&&) noexcept;

    // server
    void bind(sockaddr_in& address);
    void listen(int connections = MAX_CONN);
    ssocket accept();

    // client
    int connect(sockaddr_in& address);

    size_t send(std::string data);
    std::string read();

    int get_desc();

    void unblock();

    ~ssocket();

  private:
    static constexpr int MAX_CONN = 1000;
    static constexpr size_t BUFF_SIZE = 1024;
    explicit ssocket(int fd);

    int descriptor;
    int flags;
};

#endif // SOCKETWRAP
