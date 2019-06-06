#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

const int MAX_EVENTS = 64;
const size_t BUFFER_SIZE = 1024;

int sock, epollfd;
struct epoll_event event, events[MAX_EVENTS];

void connect(const char *address, const uint16_t port) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        throw std::runtime_error("Can't init socket");
    }
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(address);
//    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("connect");
        throw std::runtime_error("Can't make connection");
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    event.events = EPOLLIN;
    event.data.fd = sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    event.events = EPOLLIN;
    event.data.fd = 0;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

void make_request() {
    bool action = true;
    while (action) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == sock) {
                std::vector<char> response(BUFFER_SIZE);

                ssize_t all_received = 0;
                while (all_received < BUFFER_SIZE) {
                    ssize_t received = recv(sock, response.data() + all_received, BUFFER_SIZE, 0);
                    all_received += received;
                    if (received == -1) {
                        perror("recv");
                        throw std::runtime_error("Can't receive request");
                    }
                    if (response.back() == '\0') {
                        break;
                    }
                }

                if (all_received > 0) {
                    response.resize(all_received);
                    std::cout << "Response: " << response.data() << std::endl;
                } else {
                    if (all_received == -1) {
                        perror("recv");
                        throw std::runtime_error("Can't receive response");
                    } else if (all_received == 0) {
                        action = false;
                        continue;
                    }
                }
            }

            if (events[i].data.fd == 0) {
                std::string message;
                std::getline(std::cin, message);
                if (message == "ex") {
                    action = false;
                } else {
                    ssize_t all_sended = 0;
                    while (all_sended < std::min(BUFFER_SIZE, message.size() + 1)) {
                        ssize_t sended = send(sock, message.data() + all_sended, BUFFER_SIZE, 0);
                        if (sended == -1) {
                            perror("send");
                            throw std::runtime_error("Can't send request");
                        }
                        all_sended += sended;
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
        connect(argv[1], static_cast<uint16_t>(std::stoul(argv[2])));
        make_request();
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid port" << std::endl;
        exit(EXIT_FAILURE);
    }

    return 0;
}