#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <cstring>
#include <set>

const int BUFFER_SIZE = 1024;
const int EPOLL_SIZE = 1024;
const int EPOLL_RUN_TIMEOUT = -1;

int open_socket() {
    int listener;
    listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listener < 0) {
        std::cerr << "Can't create a socket: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        std::cerr << "Can't set options on socket: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    return listener;
}

void set_non_blocking(int listener) {
    if (fcntl(listener, F_SETFL, fcntl(listener, F_GETFD, 0) | O_NONBLOCK) < 0) {
        std::cerr << "Can't set listener to non-blocking mod: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void binding(int listener, int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listener, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        std::cerr << "Can't bind socket: " << strerror(errno) << std::endl;
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

void listening(int listener) {
    if (listen(listener, 1) < 0) {
        std::cerr << "Listening failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void add_listener_to_epoll(int epoll, int listener, epoll_event &event) {
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, listener, &event) < 0) {
        std::cerr << "Can't add listener to epoll: " << strerror(errno) << std::endl;
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

int accepting(int listener) {
    int sock = accept(listener, nullptr, nullptr);
    if (sock < 0) {
        std::cerr << "Can't accept connection: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    return sock;
}

void add_client_to_epoll(int epoll, int client, epoll_event &event) {
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, client, &event) < 0) {
        std::cerr << "Can't add client to epoll: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void close_connection(int client) {
    std::cout << "Client " << client << " closed connection" << std::endl;
    if (close(client) < 0) {
        std::cerr << "Can't close connection: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void resend_message(int client) {
    char message[BUFFER_SIZE];
    memset(message, 0, sizeof(message));
    ssize_t len;
    if ((len = recv(client, message, BUFFER_SIZE, 0)) < 0) {
        std::cerr << "Recv failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    message[len] = '\0';
    if (len == 0) {
        close_connection(client);
    } else {
        send(client, message, (size_t) len, 0);
    }
}

void send_welcome_message(int client) {
    std::string message = "Welcome to echo server!\n";
    send(client, message.data(), BUFFER_SIZE, 0);
}

int main(int argc, char *argv[]) {
    int port;
    int tries = 100;
    if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        std::cerr << "Not enough arguments. One argument required: [port]" << std::endl;
        exit(EXIT_FAILURE);
    }
    int listener = open_socket();
    set_non_blocking(listener);
    binding(listener, port);
    listening(listener);
    int epoll = create_epoll(EPOLL_SIZE);
    static struct epoll_event event, events[EPOLL_SIZE];
    event.events = EPOLLIN;
    event.data.fd = listener;
    add_listener_to_epoll(epoll, listener, event);
    std::cout << "Server is ready" << std::endl;
    for (int i = 0; i < tries; ++i) {
        int epoll_events_count = waiting(epoll, events, EPOLL_SIZE, EPOLL_RUN_TIMEOUT);
        for (int cur_event = 0; cur_event < epoll_events_count; ++cur_event) {
            if (events[cur_event].data.fd == listener) {
                int client = accepting(listener);
                std::cout << "Client #" << client << " connected" << std::endl;
                set_non_blocking(client);
                event.data.fd = client;
                add_client_to_epoll(epoll, client, event);
                send_welcome_message(client);
            } else {
                resend_message(events[cur_event].data.fd);
            }
        }
    }
    close(listener);
    close(epoll);
    exit(EXIT_SUCCESS);
}