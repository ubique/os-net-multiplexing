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

struct server_exception : std::runtime_error {
    explicit server_exception(const std::string &cause)
            : std::runtime_error(cause + ": " + strerror(errno)){}

};

class vector;

class Server {
public:
    Server() = default;

    Server(char* address, uint16_t port) : socket(Socket()) {
        memset(&server_addr, 0, sizeof(sockaddr_in));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(address);
        server_addr.sin_port = port;
        try {
            socket.bind(server_addr);
            socket.listen(3);
        } catch (socket_exception &e) {
            throw server_exception(e.what());
        }
        try {
            epoll.start();
            epoll.check_ctl(socket.getFd(), EPOLLIN, EPOLL_CTL_ADD);
        } catch (epoll_exception &e) {
            throw server_exception(e.what());
        }
    }


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

    void run() {
        struct epoll_event events[EVENTS_SIZE];
        std::map<int, std::vector<std::string>> answers;
        while(true) {
            try {
                int n = epoll.wait(events);
                for (int i = 0; i < n; ++i) {
                    int cur_fd = events[i].data.fd;
                    uint32_t cur_events = events[i].events;
                    if (cur_events & EPOLLIN) {
                        if (cur_fd == socket.getFd()) {
                            try {
                                Socket client_socket = socket.accept();
                                client_socket.unblock();
                                epoll.check_ctl(client_socket.getFd(), EPOLLIN | EPOLLERR | EPOLLHUP, EPOLL_CTL_ADD);
                                std::cout << "Client connected" << std::endl;
                            } catch (std::runtime_error &e) {
                                std::cerr << e.what() << std::endl;
                                continue;
                            }
                        } else {
                            Socket client_socket = Socket(cur_fd);
                            std::string ans = client_socket.recv();
                            if (answers[client_socket.getFd()].empty) {
                                epoll.check_ctl(client_socket.getFd(), EPOLLIN | EPOLLERR | EPOLLHUP, EPOLL_CTL_MOD);
                            }
                            answers[client_socket.getFd()].push_back(ans);
                        }
                    }
                    if (cur_events & EPOLLOUT) {
                        try {
                            Socket client_socket = socket.accept();
                            client_socket.send(answers[client_socket.getFd()].back());
                            answers[client_socket.getFd()].pop_back();
                            if (answers[client_socket.getFd()].empty()) {
                                epoll.check_ctl(client_socket.getFd(), EPOLLIN | EPOLLERR | EPOLLHUP, EPOLL_CTL_MOD);
                            }
                        } catch (socket_exception &e) {
                            std::cerr << e.what() << std::endl;
                        }
                    }
                    if (cur_events & (EPOLLERR | EPOLLHUP)) {
                        epoll.check_ctl(cur_fd, -1, EPOLL_CTL_DEL);
                        std::cout << "Client disconnected" << std::endl;
                        if (answers.count(cur_fd)) {
                            answers.erase(cur_fd);
                        }
                    }
                }
            } catch (std::runtime_error &e) {
                try {
                    epoll.check_ctl(socket.getFd(), -1, EPOLL_CTL_DEL);
                    socket.close();
                } catch (std::runtime_error &e) {
                    std::cerr << e.what() << std::endl;
                    throw server_exception(e.what());
                }
            }
        }
    }

#pragma clang diagnostic pop

    ~Server() = default;


private:
    struct sockaddr_in server_addr{};
    Socket socket;
    Epoll epoll;

    const int EVENTS_SIZE = 1024;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Two arguments expected: address and port.";
        exit(EXIT_FAILURE);
    }
    try {
        Server server(argv[1], static_cast<uint16_t>(std::stoul(argv[2])));
        server.run();
    } catch (server_exception &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}
