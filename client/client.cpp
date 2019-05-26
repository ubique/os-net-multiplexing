#include "client.h"

std::string get_in() {
    std::vector<char> buffer(1024);
    ssize_t was_read = read(STDOUT_FILENO, buffer.data(), 1024);
    if (was_read == -1) {
        throw std::runtime_error("Failed to read from stdin");
    }
    buffer.resize(static_cast<size_t>(was_read));
    return std::string(buffer.data());
}

std::string put_out(std::string const& data) {
    ssize_t was_written = write(STDOUT_FILENO, data.data(), data.size());
    if (was_written == -1) {
        throw std::runtime_error("Failed to write to stdout");
    }
    return data.substr(static_cast<size_t>(was_written));
}

client::client(char* address, uint16_t port) : client_socket() {
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = port;
    server_address.sin_addr.s_addr = inet_addr(address);

    epoll_fd = epoll_create(MAX_CONN);
    if (epoll_fd == -1) {
        throw client_exception("Cannot create epoll");
    }
    client_socket.unblock();
    epoll::add(epoll_fd, STDIN_FILENO, EPOLLIN);
    connected = true;
    stop = false;
    try {
        if (client_socket.connect(server_address) == -1) {
            if (errno == EINPROGRESS) {
                epoll::add(epoll_fd, client_socket.get_desc(),
                           EPOLLIN | EPOLLOUT);
                connected = false;
            } else {
                throw client_exception("Connection failed");
            }
        } else {
            epoll::add(epoll_fd, client_socket.get_desc(), EPOLLIN);
        }
    } catch (std::runtime_error& e) {
        throw client_exception(e.what());
    }
}

void client::interact() {
    std::string recieved, printable, sendable;
    while (!stop) {
        int evs_amount = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (evs_amount == -1) {
            throw client_exception("Epoll wait failed");
        }
        for (int i = 0; i < evs_amount; i++) {
            uint32_t evs = events[i].events;
            int current = events[i].data.fd;
            if (current == client_socket.get_desc()) {
                if ((evs & EPOLLERR) || (connected && (evs & EPOLLHUP))) {
                    throw client_exception("Server disconnected");
                }
                if (evs & EPOLLIN) {
                    recieved += client_socket.read();
                    std::string answer;
                    while (!(answer = next(recieved)).empty()) {
                        if (printable.empty()) {
                            epoll::add(epoll_fd, STDOUT_FILENO, EPOLLOUT);
                        }
                        printable += answer;
                    }
                }
                if (evs & EPOLLOUT) {
                    if (!connected) {
                        int err = 0;
                        socklen_t len = sizeof(int);
                        if (getsockopt(client_socket.get_desc(), SOL_SOCKET,
                                       SO_ERROR, &err, &len) == -1) {
                            throw client_exception(
                                "Cannot get socket error status");
                        }
                        if (err != 0) {
                            throw client_exception("Connection failed");
                        }
                        connected = true;
                        epoll::change_mode(epoll_fd, current,
                                           sendable.empty() ? 0 : EPOLLOUT);
                    } else {
                        sendable =
                            sendable.substr(client_socket.send(sendable));
                        if (sendable.empty()) {
                            epoll::change_mode(epoll_fd, current, EPOLLIN);
                        }
                    }
                }
            } else if (current == STDIN_FILENO) {
                std::string request = get_in();
                if (request.empty() || request == "exit" || std::cin.eof()) {
                    stop = true;
                    continue;
                }
                if (sendable.empty()) {
                    epoll::change_mode(epoll_fd, client_socket.get_desc(),
                                       EPOLLIN | EPOLLOUT);
                }
                sendable += request + "\r\n";
            } else if (current == STDOUT_FILENO) {
                printable = put_out(printable);
                if (printable.empty()) {
                    epoll::remove(epoll_fd, STDOUT_FILENO);
                }
            }
        }
    }
}

client::~client() { delete[] events; }
