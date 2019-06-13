#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

const int MAX_EVENTS = 64;
const int BACKLOG = 4;
const size_t BUFFER_SIZE = 1024;

int listener, epollfd;
struct epoll_event event, events[MAX_EVENTS];

void bind(const char *address, const uint16_t port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = port;
//    addr.sin_addr.s_addr = inet_addr(address);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listener == -1) {
        perror("socket");
        throw std::runtime_error("Can't init socket");
    }

    int optval = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(listener, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        throw std::runtime_error("Can't bind socket");
    }

    if (listen(listener, BACKLOG) < 0) {
        perror("listen");
        throw std::runtime_error("Error during listening");
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    event.events = EPOLLIN;
    event.data.fd = listener;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listener, &event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started" << std::endl;
}

void run() {
    while (true) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listener) {
                int sock = accept(listener, nullptr, nullptr);
                if (sock == -1) {
                    perror("accept");
                    continue;
                }

                const int flags = fcntl(sock, F_GETFL, 0);
                fcntl(sock, F_SETFL, flags | O_NONBLOCK);

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &event) == -1) {
                    perror("event_ctl");
                    close(sock);
                    exit(EXIT_FAILURE);
                }
            } else {
                std::vector<char> request(BUFFER_SIZE);
                int sock = events[i].data.fd;

                ssize_t all_received = 0;
                while (all_received < BUFFER_SIZE) {
                    ssize_t received = recv(sock, request.data() + all_received, BUFFER_SIZE, 0);
                    all_received += received;
                    if (received == -1) {
                        perror("recv");
                        throw std::runtime_error("Can't receive request");
                    }
                    if (request.back() == '\0') {
                        break;
                    }
                }

                if (all_received > 0) {
                    request.resize(all_received);
                    std::cout << "Request: " << request.data() << std::endl;
                    ssize_t all_sended = 0;
                    while (all_sended < std::min(BUFFER_SIZE, request.size() + 1)) {
                        ssize_t sended = send(sock, request.data() + all_sended, BUFFER_SIZE, 0);
                        if (sended == -1) {
                            perror("send");
                            throw std::runtime_error("Can't send response");
                        }
                        all_sended += sended;
                    }

                } else {
                    if (all_received == -1) {
                        perror("recv");
                        break;
                    } else if (all_received == 0) {
                        std::cout << "Disconnection" << std::endl;
                        break;
                    }
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, nullptr) == -1) {
                        close(sock);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Expected 2 arguments: <address> <port>" << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        bind(argv[1], static_cast<uint16_t>(std::stoul(argv[2])));
        run();
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid port" << std::endl;
        exit(EXIT_FAILURE);
    }

    return 0;
}