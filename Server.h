#ifndef OS_NET_MULTIPLEXING_SERVER_H
#define OS_NET_MULTIPLEXING_SERVER_H

#include <sys/epoll.h>

#include <cstdint>
#include <stdexcept>
#include <cstring>


struct ServerException: std::runtime_error {
    ServerException(std::string const& msg) :
            std::runtime_error(msg+ ", " + strerror(errno)) {}
};

struct Server {
    explicit Server(uint16_t port);

    void run();

private:
    struct FileDescriptor {
        FileDescriptor(int fd) : fd(fd) {
            if (fd == -1)
                throw ServerException("Invalid fd");
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

    void readStdin();
    void processRequest(int fd);
    void disconnect(int fd);
    void sendReply(int fd, int len);

    static void closeFileDescriptor(int fd);

    bool epoll_ctl_add(int efd, int fd, uint32_t events);

    const uint16_t port;
    const FileDescriptor listenfd;
    const FileDescriptor epollfd;

    static const int LISTEN_BACKLOG = 50;
    static const size_t MAX_EVENTS = 50;
    static const size_t BUFFER_SIZE = 4098;

    struct epoll_event epollEvent;
    struct epoll_event events[MAX_EVENTS];

    char requestBuffer[BUFFER_SIZE];

    bool finished = false;
};


#endif //OS_NET_MULTIPLEXING_SERVER_H
