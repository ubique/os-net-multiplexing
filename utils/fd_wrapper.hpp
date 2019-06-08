#ifndef SOCKET_WRAPPER_HPP
#define SOCKET_WRAPPER_HPP

#include "logger.hpp"

#include <unistd.h>
#include <sys/socket.h>

class fd_wrapper {

public:

    fd_wrapper();

    explicit fd_wrapper(int descriptor);

    ~fd_wrapper();

    void renew();

    int get_fd() const;

    bool check_valid() const;

    int send_message(char const *buf, size_t buffer_size, bool log_success = true);

    int receive_message(char *buf, size_t &buffer_size, unsigned int real_size, bool log_success = true);

    void close();

private:

    int descriptor;

};

#endif // SOCKET_WRAPPER_HPP
