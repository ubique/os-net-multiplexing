//
// Created by vitalya on 27.05.19.
//

#ifndef OS_NET_SOCKET_WRAPPER_H
#define OS_NET_SOCKET_WRAPPER_H

#include <sys/socket.h>
#include <sys/un.h>
#include <string>
#include <zconf.h>

class socket_wrapper {
public:
    socket_wrapper();
    socket_wrapper(const socket_wrapper& other) = delete;
    socket_wrapper(socket_wrapper&& other) noexcept;
    ~socket_wrapper();

    socket_wrapper& operator=(const socket_wrapper& other) = delete;
    socket_wrapper& operator=(socket_wrapper&& other) noexcept;


    void create(const std::string& socket_name);

    void bind(sockaddr_un& address);

    void listen();

    void connect(sockaddr_un& address);

    socket_wrapper accept();

    int read(char* buffer, int size);

    void write(const std::string& message);

    int get_fd();

private:
    explicit socket_wrapper(int fd_);

private:
    std::string name;
    int fd;
};


#endif //OS_NET_SOCKET_WRAPPER_H
