#include "server.h"

// methods

server::server(char* address, uint16_t port)
    : events(new epoll_event[MAX_EVENTS]) {
    memset(&server_address, 0, sizeof(sockaddr_in));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(address);
    server_address.sin_port = port;

    epoll_fd = epoll_create(MAX_CONN);
    if (epoll_fd == -1) {
        throw server_exception("Cannot create epoll descriptor");
    }

    ssocket server_socket;
    server_desc = server_socket.get_desc();
    server_socket.bind(server_address);
    server_socket.listen();
    try {
        epoll::add(epoll_fd, server_socket.get_desc(), EPOLLIN);
    } catch (std::runtime_error& e) {
        throw server_exception(e.what());
    }
    sockets[server_socket.get_desc()] = std::move(server_socket);
}

[[noreturn]] void server::run() {
    while (true) {
        int evs_amount = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (evs_amount == -1) {
            throw server_exception("Epoll wait failed");
        }
        for (int i = 0; i < evs_amount; i++) {
            uint32_t evs = events[i].events;
            int current = events[i].data.fd;
            if (evs & (EPOLLERR | EPOLLHUP)) {
                epoll::remove(epoll_fd, current);
                recieved.erase(current);
                sending.erase(current);
                continue;
            }
            if (evs & EPOLLIN) {
                if (current == server_desc) {
                    ssocket new_client = sockets[current].accept();
                    epoll::add(epoll_fd, new_client.get_desc(), EPOLLIN);
                    sockets[new_client.get_desc()] = std::move(new_client);
                } else {
                    recieved[current] += sockets[current].read();
                    std::string request;
                    while (!(request = next(recieved[current])).empty()) {
                        if (!sending.count(current)) {
                            epoll::change_mode(epoll_fd, current,
                                               EPOLLIN | EPOLLOUT);
                        }
                        sending[current] += request + "\r\n";
                    }
                }
            }
            if (evs & EPOLLOUT) {
                sending[current] = sending[current].substr(
                    sockets[current].send(sending[current]));
                if (sending[current].empty()) {
                    sending.erase(current);
                    epoll::change_mode(epoll_fd, current, EPOLLIN);
                }
            }
        }
    }
}

server::~server() { delete[] events; }
