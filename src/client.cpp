#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

static const size_t BUFFER_SIZE = 4096;
static const size_t MAX_EVENTS = 2;

struct fd_wrapper {
    int fd;
    explicit fd_wrapper(int fd) : fd(fd) {}

    operator int() { return fd; }

    fd_wrapper(fd_wrapper const &) = delete;

    ~fd_wrapper()
    {
        if (fd != -1 && close(fd) == -1) {
            std::cerr << "close failed: " << strerror(errno) << std::endl;
        }
    }
};

void run(std::string const &address, std::string const &port_representation)
{
    struct sockaddr_in server;
    char server_reply[BUFFER_SIZE];
    int port = stoi(port_representation);

    fd_wrapper socket_fd(socket(AF_INET, SOCK_STREAM, 0));
    if (socket_fd == -1) {
        throw std::runtime_error("Cannot create socket");
    }

    server.sin_addr.s_addr = inet_addr(address.data());
    server.sin_family = AF_INET;
    server.sin_port = htons(static_cast<uint16_t>(port));

    if (connect(socket_fd, reinterpret_cast<sockaddr *>(&server),
                sizeof(server)) == -1) {
        throw std::runtime_error("Cannot connect to server");
    }
    struct epoll_event main_event, events[MAX_EVENTS];

    fd_wrapper epollfd(epoll_create1(0));
    if (epollfd == -1) {
        throw std::runtime_error("Cannot create epoll");
    }
    main_event.events = EPOLLIN;
    main_event.data.fd = socket_fd;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socket_fd, &main_event) == -1) {
        throw std::runtime_error("Cannot add server to epoll_ctl");
    }

    main_event.events = EPOLLIN;
    main_event.data.fd = 0;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &main_event) == -1) {
        throw std::runtime_error("Cannot add stdin to epoll_ctl");
    }

    std::string query;
    bool live = true;
    while (live) {
        int available = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (available == -1) {
            throw std::runtime_error("epoll_wait failed");
        }

        for (int i = 0; i < available; ++i) {
            if (events[i].data.fd == socket_fd) {
                ssize_t received =
                    recv(socket_fd, server_reply, BUFFER_SIZE, 0);
                if (received < 0) {
                    throw std::runtime_error(
                        "Error while receiving data from server");
                }
                if (received == 0) {
                    live = false;
                    continue;
                }
                std::cout << std::string{server_reply,
                                         static_cast<size_t>(received)}
                          << std::endl;
            }
            if (events[i].data.fd == 0) {
                getline(std::cin, query);
                if (query == "EXIT" || !std::cin) {
                    live = false;
                    continue;
                }
                if (send(socket_fd, query.data(), query.size(), 0) == -1) {
                    throw std::runtime_error(
                        "Error while sending data to server");
                }
            }
        }
    }
}

static const std::string greeting =
    R"BLOCK(
Echo client with reading stdin and listening to server via epoll.
Usage: server [address [port]]
Default address is 127.0.0.1
Default port is 8888
You can send messages and get them back.
Also you can type 'EXIT' to exit this client

)BLOCK";

int main(int argc, char *argv[])
{
    std::cout << greeting << std::endl;
    std::string address = argc < 2 ? "127.0.0.1" : std::string{argv[1]};
    std::string port = argc < 3 ? "8888" : std::string{argv[2]};
    try {
        run(address, port);
    } catch (const std::exception &e) {
        std::cerr << e.what();
        if (errno != 0) {
            std::cerr << ": " << strerror(errno);
        }
        std::cerr << std::endl;
    }
}
