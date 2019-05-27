#ifndef OS_NET_MULTIPLEXING_CLIENT_H
#define OS_NET_MULTIPLEXING_CLIENT_H

#include <sys/epoll.h>

#include <cstdint>
#include <stdexcept>
#include <cstring>

struct ClientException: std::runtime_error {
    ClientException(std::string const& msg) :
            std::runtime_error(msg+ ", " + strerror(errno)) {}
};

struct Client {
    explicit Client(uint16_t port);

    void run();

private:
    struct FileDescriptor {
        FileDescriptor(int fd) : fd(fd) {
            if (fd == -1)
                throw ClientException("Invalid fd");
        }

        ~FileDescriptor() {
            closeFileDescriptor(fd);
        }

        operator int() const {
            return fd;
        }

    private:
        const int fd;
    };

    static void closeFileDescriptor(int fd);
private:
    void getResponse();
    void checkStdin();
    void sendData(std::string const& line);

    void epoll_ctl_add(int efd, int fd, uint32_t ev);

    const uint16_t port;
    const FileDescriptor sfd;
    const FileDescriptor epollfd;

    static const int LISTEN_BACKLOG = 50;
    static const size_t MAX_EVENTS = 2;
    static const size_t BUFFER_SIZE = 4098;

    struct epoll_event epollEvent;
    struct epoll_event events[MAX_EVENTS];

    char responseBuffer[BUFFER_SIZE];

    bool finished = false;
};


#endif //OS_NET_MULTIPLEXING_CLIENT_H
