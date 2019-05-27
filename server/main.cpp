#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "util/EventManager.h"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <util/ConsoleHandler.h>


using namespace std;


class ClientHandler : public IHandler {
public:
    static const int BUFFER_SIZE = 1024;

    explicit ClientHandler(int fd) : client_socket(fd) {
        cout << "Client connected: " << fd << endl << endl;
    }

    void handleData(EventManager &waiter) override {
        uint8_t buffer[BUFFER_SIZE];
        int bytes = read(client_socket, buffer, BUFFER_SIZE);

        if (bytes == -1) {
            throw HandlerException("Cannot read from client.", errno);
        } else if (bytes == 0) {
            cout << "Client disconnected: " << client_socket << endl << endl;
            waiter.deleteHandler(client_socket);
        } else {
            string query(buffer, buffer + bytes);
            cout << "Client sends: " << client_socket << endl
                 << "    " << query << endl << endl;

            if (send(client_socket, buffer, bytes, 0) == -1) {
                throw HandlerException("Cannot send response.", errno);
            }
        }
    }

    void handleError(EventManager &waiter) override {
        error_t error = getError(client_socket);
        waiter.deleteHandler(client_socket);
        if (error != 0) {
            throw HandlerException("Client handler failed.", error);
        }
    }

    int getFD() override { return client_socket; }

    ~ClientHandler() override { close(client_socket); }

private:
    int client_socket;
};


class ClientAcceptor : public IHandler {
public:
    static const int MAX_PENDING = 228;

    explicit ClientAcceptor(in_port_t port) {
        listen_socket = socket(PF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = PF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(listen_socket, (sockaddr *) &addr, sizeof(addr)) == -1) {
            throw HandlerException("Cannot bind.", errno);
        }
        if (listen(listen_socket, MAX_PENDING) == -1) {
            throw HandlerException("Cannot listen.", errno);
        }
    }

    void handleData(EventManager &waiter) override {
        sockaddr_in clientAddr{};
        socklen_t len;
        int client_socket = accept(listen_socket, (sockaddr *) &clientAddr, &len);
        if (client_socket == -1) {
            throw HandlerException("Cannot accept client", errno);
        }
        const int flags = fcntl(client_socket, F_GETFL, 0);
        fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
        shared_ptr<IHandler> handler(new ClientHandler(client_socket));
        waiter.addHandler(handler);
    }

    void handleError(EventManager &waiter) override {
        error_t error = getError(listen_socket);
        waiter.deleteAll();
        if (error != 0) {
            throw HandlerException("Listener failed.", error);
        }
    }

    int getFD() override { return listen_socket; }

    ~ClientAcceptor() override { close(listen_socket); }

private:
    int listen_socket;
};


class ServerConsole : public ConsoleHandler {
public:
    void handleData(EventManager &waiter) override {
        std::string command;
        std::cin >> command;
        if (command == "exit") {
            waiter.deleteAll();
        }
    }
};


int main(int argc, char *argv[]) {
    const in_port_t port = stoi(argv[1]);
    shared_ptr<IHandler> acceptor(new ClientAcceptor(port));
    shared_ptr<IHandler> consoleHandler(new ServerConsole());
    EventManager epollWaiter;
    epollWaiter.addHandler(acceptor);
    epollWaiter.addHandler(consoleHandler);
    epollWaiter.wait();
}