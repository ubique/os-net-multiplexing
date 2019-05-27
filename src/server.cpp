#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>

static const size_t BUFFER_SIZE = 4096;
static const size_t MAX_EVENTS = 1024;

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
    int port = stoi(port_representation);
    fd_wrapper server_socket(socket(AF_INET, SOCK_STREAM, 0));
    if (server_socket == -1) {
        throw std::runtime_error("Cannot create socket");
    }
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(address.data());
    server.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server),
             sizeof(server)) == -1) {
        throw std::runtime_error("Cannot bind");
    }

    if (listen(server_socket, 3) == -1) {
        throw std::runtime_error("Cannot listen");
    }

    struct epoll_event main_event, events[MAX_EVENTS];

    fd_wrapper epollfd(epoll_create1(0));
    if (epollfd == -1) {
        throw std::runtime_error("Cannot create epoll instance");
    }
    main_event.events = EPOLLIN;
    main_event.data.fd = server_socket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_socket, &main_event) == -1) {
        throw std::runtime_error("Cannot add into epoll ctl");
    }

    char message_buffer[BUFFER_SIZE];
    while (true) {
        int available = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (available == -1) {
            throw std::runtime_error("Waiting failed");
        }
        for (int i = 0; i < available; ++i) {
            if (events[i].data.fd == server_socket) {
                struct sockaddr_in client;
                size_t socket_size = sizeof(struct sockaddr_in);
                int client_socket =
                    accept(server_socket, reinterpret_cast<sockaddr *>(&client),
                           reinterpret_cast<socklen_t *>(&socket_size));
                if (client_socket == -1) {
                    std::cerr
                        << "Could not connect to client: " << strerror(errno)
                        << std::endl;
                    continue;
                }
                std::cout << "Connected" << std::endl;
                int flags = fcntl(client_socket, F_GETFL, 0);
                fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
                main_event.events = EPOLLIN | EPOLLET;
                main_event.data.fd = client_socket;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_socket,
                              &main_event) == -1) {
                    std::cerr << "Could not add client into epoll_ctl: "
                              << strerror(errno) << std::endl;
                    if (close(client_socket) == -1) {
                        std::cerr << "Close failed: " << strerror(errno)
                                  << std::endl;
                    }
                }
            } else {
                int client_socket = events[i].data.fd;
                ssize_t read_size =
                    recv(client_socket, message_buffer, BUFFER_SIZE, 0);
                if (read_size <= 0) {
                    if (read_size == -1) {
                        std::cerr << "Could not read: " << strerror(errno)
                                  << std::endl;
                    }
                    std::cout << "Disconnected" << std::endl;
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, client_socket,
                                  nullptr) == -1) {
                        if (close(client_socket) == -1) {
                            std::cerr << "Close failed: " << strerror(errno)
                                      << std::endl;
                        }
                    }
                    continue;
                }
                std::cout << "[RECEIVED] "
                          << std::string{message_buffer,
                                         static_cast<size_t>(read_size)}
                          << std::endl;
                if (write(client_socket, message_buffer,
                          static_cast<size_t>(read_size)) == -1) {
                    std::cerr << "Could not write response: " << strerror(errno)
                              << std::endl;
                }
            }
        }
    }
}

static const std::string greeting =
    R"BLOCK(
Echo server with multiple client support via epoll.
Usage: server [address [port]]
Default address is 127.0.0.1
Default port is 8888

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
