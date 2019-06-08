#include "utils.h"
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace utils {
    void epoll_create(int& epoll) {
        epoll = ::epoll_create1(0);
        if (epoll == -1) {
            perror("Cannot create epoll");
            exit(EXIT_FAILURE);
        }
    }

    bool accept(int& resultfd, int sockfd, sockaddr* addr, socklen_t* len) {
        resultfd = ::accept(sockfd, addr, len);
        if (resultfd == -1) {
            perror("Cannot accept connection");
            return false;
        }
        return true;
    }

    void connect(int& sockfd, sockaddr_in& server) {
        if (::connect(sockfd, reinterpret_cast<sockaddr*>(&server), sizeof(sockaddr_in)) == -1) {
            perror("Cannot connect to server");
            exit(EXIT_FAILURE);
        }
    }

    void bind_and_listen(int& sockfd, sockaddr_in& server) {
        if (bind(sockfd, reinterpret_cast<sockaddr*>(&server), sizeof(sockaddr_in)) == -1) {
            perror("Cannot bind to socket");
            exit(EXIT_FAILURE);
        }
        if (listen(sockfd, BACKLOG) == -1) {
            perror("Cannot listen socket");
            exit(EXIT_FAILURE);
        }
    }

    void init(int& sockfd, sockaddr_in& server, const std::string& addr, const std::string& port, int type) {
        sockfd = socket(AF_INET, type, 0);
        if (sockfd == -1) {
            perror("Cannot create socket\n");
            exit(EXIT_FAILURE);
        }
        server.sin_family = AF_INET;
        if (inet_aton(addr.data(), &server.sin_addr) == 0) {
            printf("Server's address is invalid\n");
            exit(EXIT_FAILURE);
        }
        try {
            server.sin_port = std::stoul(port);
        } catch (std::invalid_argument& e) {
            printf("Server's port is invalid\n");
            exit(EXIT_FAILURE);
        }
    }

    bool send(int sockfd, char* message, ssize_t size) {
        ssize_t count = 0;
        while (count < size) {
            ssize_t sent = ::send(sockfd, message + count, size - count, 0);
            if (sent == -1) {
                perror("Error during sending");
                return false;
            }
            count += sent;
        }
        return true;
    }

    bool recv(int sockfd, char* message, ssize_t size) {
        ssize_t count = 0;
        while (count < size) {
            ssize_t read = ::recv(sockfd, message, size, 0);
            if (read == -1 || read == 0) {
                perror("Error during reading");
                return false;
            }
            count += read;
        }
        return true;
    }

    void close(int sockfd) {
        if (::close(sockfd) == -1) {
            perror("Cannot close socket");
        }
    }

    void abort(int epoll, int sockfd) {
        printf("Aborting connection\n");
        if (::epoll_ctl(epoll, EPOLL_CTL_DEL, sockfd, nullptr) == -1) {
            perror("Cannot delete from epoll");
        }
        close(sockfd);
    }

    bool add(int epoll, int sockfd, epoll_event* ev) {
        ev->events = EPOLLIN;
        ev->data.fd = sockfd;
        if (epoll_ctl(epoll, EPOLL_CTL_ADD, sockfd, ev) == -1) {
            perror("Cannot add in epoll ctl");
            close(sockfd);
            return false;
        }
        return true;
    }
}
