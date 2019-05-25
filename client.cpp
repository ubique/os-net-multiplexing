#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <sys/epoll.h>
#include <cstring>

const int BUFFER_SIZE = 1024;
const int EPOLL_SIZE = 1024;
const int EPOLL_RUN_TIMEOUT = -1;
const int PIPE_SIZE = 2;

bool shutdown_client = false;

int open_socket() {
    int sock;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Can't create a socket: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Can't set options on socket: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    return sock;
}

void establish_connection(int sock, int port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        std::cerr << "Connect failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void create_pipe(int pipe_fd[PIPE_SIZE]) {
    if (pipe(pipe_fd) < 0) {
        std::cerr << "Can't create pipe: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

int create_epoll(int size) {
    int epoll_fd = epoll_create(size);
    if (epoll_fd < 0) {
        std::cerr << "Can't create epoll: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    return epoll_fd;
}

std::string get_request() {
    std::string request;
    std::getline(std::cin, request);
    return request;
}

void add_server_to_epoll(int epoll, int sock, epoll_event &event) {
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, sock, &event) < 0) {
        std::cerr << "Can't add client to epoll: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void send_message(int to, const char *data, unsigned long size) {
    if (write(to, data, size) < 0) {
        std::cerr << "Can't send message: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

int waiting(int epoll, epoll_event *event, int size, int time) {
    int count = epoll_wait(epoll, event, size, time);
    if (count < 0) {
        std::cerr << "Epoll wait failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    return count;
}

void close_connection(int client) {
    std::cout << "Client " << client << " closed connection." << std::endl;
    if (close(client) < 0) {
        std::cerr << "Can't close connection: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void connect_to_server(int sock) {
    ssize_t len;
    char message[BUFFER_SIZE];
    if ((len = recv(sock, message, BUFFER_SIZE, 0)) < 0) {
        std::cerr << "Recv failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    message[len] = '\0';
    if (len == 0) {
        std::cout << "Can't connect to server. Shutting down client." << std::endl;
        close_connection(sock);
        shutdown_client = true;
    } else {
        std::cout << "[SERVER]: " << message << std::endl;
    }
}

int main(int argc, char **argv) {
    int port;
    if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        std::cerr << "Not enough arguments. One argument required: [port]." << std::endl;
        exit(EXIT_FAILURE);
    }
    int sock = open_socket();
    establish_connection(sock, port);

    int pipe_fd[PIPE_SIZE];
    create_pipe(pipe_fd);

    int epoll = create_epoll(EPOLL_SIZE);
    static struct epoll_event event, events[EPOLL_SIZE];
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = sock;

    add_server_to_epoll(epoll, sock, event);
    event.data.fd = pipe_fd[0];
    add_server_to_epoll(epoll, pipe_fd[0], event);

    connect_to_server(sock);

    int pid = fork();
    if (pid < 0) {
        std::cerr << "Can't fork process: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        close(pipe_fd[0]);
        std::cout << "Enter 'exit' to shutdown client." << std::endl;
        while (!shutdown_client) {
            std::string request = get_request();
            if (request == "exit") {
                shutdown_client = true;
            } else {
                send_message(pipe_fd[1], request.data(), request.size());
            }
        }
    } else {
        close(pipe_fd[1]);
        while (!shutdown_client) {
            int epoll_events_count = waiting(epoll, events, EPOLL_SIZE, EPOLL_RUN_TIMEOUT);
            for (int cur_event = 0; cur_event < epoll_events_count; ++cur_event) {
                char message[BUFFER_SIZE];
                if (events[cur_event].data.fd == sock) {
                    ssize_t len;
                    if ((len = recv(sock, message, BUFFER_SIZE, 0)) < 0) {
                        std::cerr << "Recv failed: " << strerror(errno) << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if (len == 0) {
                        close_connection(sock);
                        shutdown_client = true;
                    } else {
                        std::cout << "[SERVER]: " << message << std::endl;
                    }
                } else {
                    ssize_t len;
                    if ((len = read(events[cur_event].data.fd, message, BUFFER_SIZE)) < 0) {
                        std::cerr << "Read failed: " << strerror(errno) << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    message[len] = '\0';
                    if (len == 0) {
                        shutdown_client = true;
                    } else {
                        send(sock, message, (size_t)len, 0);
                    }
                }
            }
        }
    }
    if (pid) {
        close(pipe_fd[0]);
        close(sock);
    } else {
        close(pipe_fd[1]);
    }
    exit(EXIT_SUCCESS);
}