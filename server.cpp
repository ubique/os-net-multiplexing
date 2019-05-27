#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/epoll.h>

const int PROTOCOL_FAMILY = AF_INET;
const int BUFFER_SIZE = 1024;
const int EPOLL_SIZE = 1024;

const int ATTEMPT_NUMBER = 5;
const int PORT = 8080;

void printErr(const std::string& message) {
    fprintf(stderr, "ERROR %s: %s\n", message.c_str(), strerror(errno));
}

void printUsage() {
    printf("Usage: <port>\n"
           "Default port: 8080\n\n");
}

void printGreetings() {
    printf("Echo server is started.\n\n");
}

int openSocket() {
    int openedSocket = socket(PROTOCOL_FAMILY, SOCK_STREAM, 0);

    if (openedSocket < 0) {
        printErr("Unable to open socket");
        exit(EXIT_FAILURE);
    }

    return openedSocket;
}

int createEpoll(int size) {
    int epoll = epoll_create(size);

    if (epoll < 0) {
        printErr("Unable to create epoll");
        exit(EXIT_FAILURE);
    }

    return epoll;
}

void bind(int listener, int port) {
    sockaddr_in serverAddress{};

    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_family = PROTOCOL_FAMILY;
    serverAddress.sin_port = htons(port);

    if (bind(listener, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(sockaddr_in)) < 0) {
        printErr("Unable to bind");
        close(listener);
        exit(EXIT_FAILURE);
    }
}

void listen(int listener) {
    if (listen(listener, ATTEMPT_NUMBER) < 0) {
        printErr("Unable to listen");
        close(listener);
        exit(EXIT_FAILURE);
    }
}

void addEpoll(int epoll, int socket, epoll_event& event) {
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, socket, &event) < 0) {
        printErr("Unable to add to epoll");
        exit(EXIT_FAILURE);
    }
}

int wait(int epoll, epoll_event* event, int size) {
    int count = epoll_wait(epoll, event, size, -1);

    if (count < 0) {
        printErr("Epoll is unable to wait");
        exit(EXIT_FAILURE);
    }

    return count;
}

int accept(int socket) {
    sockaddr_in client{};
    socklen_t clientSize;

    int accepted = accept(socket, reinterpret_cast<sockaddr*>(&client), &clientSize);
    if (accepted < 0) {
        printErr("Unable to accept");
        close(accepted);
        exit(EXIT_FAILURE);
    }

    return accepted;
}

void sendMessage(int socket) {
    char buffer[BUFFER_SIZE] = {};

    int readed = read(socket, buffer, BUFFER_SIZE);
    if (readed < 0) {
        printErr("Unable to read");
        close(socket);
        exit(EXIT_FAILURE);
    }

    if (send(socket, buffer, readed, 0) != readed) {
        printErr("Unable to send");
    }

    close(socket);
}

void echo(int listener, int port) {
    bind(listener, port);

    listen(listener);

    int epoll = createEpoll(EPOLL_SIZE);

    struct epoll_event event{}, events[EPOLL_SIZE];

    event.events = EPOLLIN | EPOLLET;
    event.data.fd = listener;

    addEpoll(epoll, listener, event);

    for (int i = 0; i < ATTEMPT_NUMBER; ++i) {
        int events_counter = wait(epoll, events, EPOLL_SIZE);
        for (int current = 0; current < events_counter; ++current) {
            if (events[current].data.fd == listener) {
                int acceptedFd = accept(listener);

                event.data.fd = acceptedFd;
                addEpoll(epoll, acceptedFd, event);

                sendMessage(acceptedFd);
            } else {
                std::string again = "Try again, please.\n";
                send(events[current].data.fd, again.data(), BUFFER_SIZE, 0);
            }
        }

    }

    close(epoll);
}

int main(int argc, char** argv) {
    printGreetings();
    printUsage();
    int port = PORT;

    if (argc == 2) {
        port = atoi(argv[1]);
    }

    int listener = openSocket();

    echo(listener, port);

    close(listener);
    exit(EXIT_SUCCESS);
}