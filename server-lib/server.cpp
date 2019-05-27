//
// Created by Михаил Терентьев on 2019-05-23.
//
#include "server.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include "server_execption.h"

void printCerr(std::string m) {
    std::cerr << m << " : " << strerror(errno) << std::endl;
}

server::server(std::string const &address, std::string const &port_representation) : epoll_fd(epoll_create1(0)),
                                                                                     socket_fd(socket(AF_INET,
                                                                                                          SOCK_STREAM,
                                                                                                          0)) {
    int port = stoi(port_representation);
    if (socket_fd.isBroken()) {
        throw server_exception("Unable to create socket");
    }
    fillAddress(address,port);
    if (bind(socket_fd.getDescriptor(), reinterpret_cast<sockaddr *>(&server_in), sizeof(server_in)) == -1) {
        throw server_exception("Unable to bind");
    }

    if (listen(socket_fd.getDescriptor(), 3) == -1) {
        throw server_exception("Unable to listen");
    }
    if (epoll_fd.isBroken()) {
        throw server_exception("Unable to create epoll instance");
    }
    main_event.events = EPOLLIN;
    main_event.data.fd = socket_fd.getDescriptor();
    if (epoll_ctl(epoll_fd.getDescriptor(), EPOLL_CTL_ADD, socket_fd.getDescriptor(), &main_event) == -1) {
        server_exception("Unable to add into epoll_ctl");
    }
}


[[noreturn]]  void server::run() {
    std::cout<<"Server started"<<std::endl;
    char buffer[BF_SZ];
    while (true) {
        int dcnt = epoll_wait(epoll_fd.getDescriptor(), events, MAX_EVENTS, -1);
        if (dcnt == -1) {
            throw server_exception("epoll_wait error");
        }
        for (int i = 0; i < dcnt; ++i) {
            if (events[i].data.fd == socket_fd.getDescriptor()) {
                struct sockaddr_in client{};
                size_t socket_size = sizeof(struct sockaddr_in);
                int cl_socket_status = accept(socket_fd.getDescriptor(), reinterpret_cast<sockaddr *>(&client),
                                           reinterpret_cast<socklen_t *>(&socket_size));
                if (cl_socket_status == -1) {
                    printCerr("Unable to connect to client");
                    continue;
                }
                std::cout << "New connection..." << std::endl;
                fcntl(cl_socket_status, F_SETFL, fcntl(cl_socket_status, F_GETFL, 0) | O_NONBLOCK);
                main_event.events = EPOLLIN | EPOLLET;
                main_event.data.fd = cl_socket_status;
                if (epoll_ctl(epoll_fd.getDescriptor(), EPOLL_CTL_ADD, cl_socket_status, &main_event) == -1) {
                    printCerr("Could not add client into epoll_ctl");
                    if (close(cl_socket_status) == -1) {
                        printCerr("Closing error");
                    }
                }
            } else {
                int cl_socket_status = events[i].data.fd;
                ssize_t read_size = recv(cl_socket_status, buffer, BF_SZ, 0);
                if (read_size <= 0) {
                    if (read_size == -1) {
                        printCerr("Receiving error");
                    }
                    std::cout << "Disconnected..." << std::endl;
                    if (epoll_ctl(epoll_fd.getDescriptor(), EPOLL_CTL_DEL, cl_socket_status, nullptr) == -1) {
                        if (close(cl_socket_status) == -1) {
                            printCerr("Closing error");
                        }
                    }
                    continue;
                }
                std::cout << "new request: " << std::string{buffer, static_cast<size_t>(read_size)} << std::endl;
                if (write(cl_socket_status, buffer, static_cast<size_t>(read_size)) == -1) {
                    printCerr("Response writer error");
                }
            }
        }
    }
}

void server::fillAddress(std::string const &address,int port) {
    server_in.sin_family = AF_INET;
    server_in.sin_addr.s_addr = inet_addr(address.data());
    server_in.sin_port = htons(static_cast<uint16_t>(port));
}