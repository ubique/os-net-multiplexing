//
// Created by anastasia on 19.05.19.
//

#include <cstring>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "socket.cpp"
#include "epoll.cpp"

struct client_exception : std::runtime_error {
    explicit client_exception(const std::string &cause)
            : std::runtime_error(cause + ": " + strerror(errno)){}

};

class Client {
public:
    Client() = default;

    Client(char* address, uint16_t port) : socket_fd(Socket()) {
        memset(&server_addr, 0, sizeof(sockaddr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(address);
        server_addr.sin_port = port;
        socket_fd.unblock();
        try {
            socket_fd.connect(server_addr);
        } catch (socket_exception &e) {
            throw client_exception(e.what());
        }
        try {
            epoll.start();
            epoll.check_ctl(socket_fd.getFd(), EPOLLIN, EPOLL_CTL_ADD);
            epoll.check_ctl(STDIN_FILENO, EPOLLIN, EPOLL_CTL_ADD);
        } catch (epoll_exception &e) {
            throw client_exception(e.what());
        }
    }

    std::string run() {
        struct epoll_event events[EPOLL_SIZE];
        std::queue<std::string> print, send;
        while (true) {
            try {
                int n = epoll.wait(events);
                for (int i = 0; i < n; ++i) {
                    uint32_t cur_events = events[i].events;
                    int cur_fd = events[i].data.fd;
                    if (cur_fd == socket_fd.getFd()) {
                        if (cur_events & EPOLLIN) {
                            std::string response = socket_fd.recv();
                            if (print.empty()) {
                                epoll.check_ctl(STDOUT_FILENO, EPOLLOUT, EPOLL_CTL_ADD);
                            }
                            print.push(response);
                        } else if (cur_events & EPOLLOUT) {
                            socket_fd.send(send.front());
                            send.pop();
                            if (send.empty()) {
                                epoll.check_ctl(cur_fd, EPOLLIN, EPOLL_CTL_MOD);
                            }
                        }
                    } else if (cur_fd == STDIN_FILENO) {
                        std::string request;
                        std::cin >> request;
                        if (send.empty()) {
                            epoll.check_ctl(socket_fd.getFd(), EPOLLIN | EPOLLOUT, EPOLL_CTL_MOD);
                        }
                        send.push(request);
                    } else {
                        std::cout << "Echo: " + print.front() << std::endl;
                        print.pop();
                        if (print.empty()) {
                            epoll.check_ctl(STDOUT_FILENO, 0, EPOLL_CTL_DEL);
                        }
                    }
                }
            } catch (std::runtime_error& e) {
                throw client_exception(e.what());
            }
        }
    }

    ~Client() = default;


private:
    struct sockaddr_in server_addr{};
    Socket socket_fd;
    Epoll epoll;

    const int EPOLL_SIZE = 1024;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Two arguments expected: address and port.";
        exit(EXIT_FAILURE);
    }
    try {
        Client client(argv[1], static_cast<uint16_t>(std::stoul(argv[2])));
        client.run();
    } catch (client_exception &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}