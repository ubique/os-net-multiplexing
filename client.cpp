#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

const int PROTOCOL_FAMILY = AF_INET;
const int BUFFER_SIZE = 1024;
const int EPOLL_SIZE = 1024;
const int PIPE_SIZE = 2;

const int PORT = 8080;
const char* IP = "127.0.0.1";


void printErr(const std::string& message) {
    fprintf(stderr, "ERROR %s: %s\n", message.c_str(), strerror(errno));
}

void printUsage() {
    printf("Usage: <port> <ip> <request>\n"
           "Default values:\n"
           "   Port: 8080\n"
           "   Address: 127.0.0.1\n"
           "   Request: request\n\n");
}

void printGreetings() {
    printf("Echo client greets you!\n\n");
}

int openSocket() {
    int openedSocket = socket(PROTOCOL_FAMILY, SOCK_STREAM, 0);

    if (openedSocket < 0) {
        printErr("Unable to open socket");
        exit(EXIT_FAILURE);
    }

    return openedSocket;
}

void connect(int socket, int port, int address) {
    sockaddr_in serverAddress{};

    serverAddress.sin_addr.s_addr = address;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_family = PROTOCOL_FAMILY;

    if (connect(socket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(sockaddr_in)) < 0) {
        printErr("Unable to connect client");
        exit(EXIT_FAILURE);
    }
}

void createPipe(int p[PIPE_SIZE]) {
    if (pipe(p) < 0) {
        printErr("Unable to create pipe");
        exit(EXIT_FAILURE);
    }
}

int createEpoll(int size) {
    int epoll = epoll_create(size);

    if (epoll < 0) {
        printErr("Unable to create epoll");
        exit(EXIT_FAILURE);
    }

    return epoll;
}

void addEpoll(int epoll, int socket, epoll_event& event) {
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, socket, &event) < 0) {
        printErr("Unable to add to epoll");
        exit(EXIT_FAILURE);
    }
}

void respond(int socket, std::string& msg) {
    const char* msgData = msg.c_str();

    size_t msgSize = strlen(msgData);
    if (send(socket, msgData, msgSize, 0) != msgSize) {
        printErr("Unable to send message");
        exit(EXIT_FAILURE);
    }

    char reply[BUFFER_SIZE] = {};
    if (read(socket, reply, BUFFER_SIZE) < 0) {
        printErr("Unable to read");
        exit(EXIT_FAILURE);
    }

    printf("Server reply: %s\n", reply);
}

int main(int argc, char** argv) {
    printGreetings();
    printUsage();

    int port = PORT;
    int address = inet_addr(IP);
    std::string request = "request";

    switch (argc) {
        case 4: {
            request = argv[3];
        }
        case 3: {
            address = inet_addr(argv[2]);
        }
        case 2: {
            port = atoi(argv[1]);
            break;
        }
        default:
            break;
    }

    int socket = openSocket();
    connect(socket, port, address);


    int p[PIPE_SIZE];
    createPipe(p);

    int epoll = createEpoll(EPOLL_SIZE);

    struct epoll_event event{}, evets[EPOLL_SIZE];

    event.events = EPOLLIN | EPOLLET;
    evets->data.fd = socket;

    addEpoll(epoll, socket, event);

    event.data.fd = p[0];
    addEpoll(epoll, p[0], event);

    respond(socket, request);

    close(epoll);
    close(socket);
    exit(EXIT_SUCCESS);
}