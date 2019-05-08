//
// Created by max on 07.05.19.
//

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <memory>
#include <vector>
#include <map>
#include <csignal>
#include <algorithm>
#include "helper.h"
#include "Epoll.h"
#include "Socket.h"

using Symb = Printer::Symbols;


std::vector<Socket> socket_list;
Epoll epoll1{};
bool flag = false;

auto find_fd_in_socket_list(int fd) {
    return std::find_if(socket_list.begin(), socket_list.end(), [fd](const Socket &soc) {
        return soc.connection_socket == fd;
    });
}


void f_signal(UNUSED int a) {
    flag = true;
    std::cout << "Exiting..." << std::endl;
}

int main(int argc, char **argv) {
    signal(SIGINT, f_signal);
    try {
        std::cout << "Socket setup" << std::endl;

        if (argc == 1) {
            std::cout << "No socket address provided" << std::endl;
            return EXIT_FAILURE;
        }
        socket_list.reserve(argc - 1);
        for (int i = 1; i < argc; ++i) {
            Socket socket_tmp;
            socket_tmp.create(argv[i], Socket::FLAG_SERVER);
            socket_tmp.bind();
            socket_tmp.listen();
            socket_list.push_back(std::move(socket_tmp));
        }

        std::cout << "Socket setup completed" << std::endl;
        std::cout << "Epoll setup" << std::endl;

        epoll1.start();
        for (int i = 1; i < argc; ++i)
            epoll1.do_event(socket_list[i - 1].connection_socket, EPOLLIN, "Add");

        std::cout << "Epoll setup completed" << std::endl;


        std::map<int, std::vector<std::string>> responses;
        while (true) {
            std::unique_ptr<struct epoll_event[]> events(new struct epoll_event[BUFFER_SIZE + 1]);
            int events_size = epoll1.wait(events.get());
            for (int i = 0; i < events_size; ++i) {
                struct epoll_event &event = events[i];
                if (auto iter = find_fd_in_socket_list(event.data.fd); iter != socket_list.end()) {
                    Printer::print(std::cout, "Add", Symb::End);
                    int data_socket = iter->accept();
                    Printer::printr(std::cout, "Accept on socket. Data socket : ", data_socket, Symb::End);
                    epoll1.do_event(data_socket, EPOLLIN | EPOLLHUP | EPOLLERR, "Add");
                } else if (event.events & (EPOLLHUP | EPOLLERR)) {
                    Printer::print(std::cout, "Close", Symb::End);
                    {
                        int tmp = event.data.fd;
                        Printer::printr(std::cout, "Closing client: ", tmp, Symb::End);
                    }
                    epoll1.do_event(event.data.fd, -1, "Del");
                } else if (event.events & EPOLLIN) {
                    Printer::print(std::cout, "In", Symb::End);
                    Socket read_socket{event.data.fd};
                    auto data = read_socket.read();

                    std::string response = std::string("Hello, ").append(data.second.get());

                    if (responses[event.data.fd].empty()) {
                        epoll1.do_event(event.data.fd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP, "Mod");
                    }
                    responses[event.data.fd].push_back(response);
                } else if (event.events & EPOLLOUT) {
                    Printer::print(std::cout, "Out", Symb::End);
                    Socket write_socket{event.data.fd};

                    write_socket.write(responses[event.data.fd].back());
                    responses[event.data.fd].pop_back();

                    if (responses[event.data.fd].empty()) {
                        epoll1.do_event(event.data.fd, EPOLLIN | EPOLLERR | EPOLLHUP, "Mod");
                    }
                }

            }
            if (flag) {
                break;
            }
        }
    } catch (...) {
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_SUCCESS);
}