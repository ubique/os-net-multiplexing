//
// Created by max on 07.05.19.
//

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <memory>
#include <vector>
#include "helper.h"
#include "Epoll.h"
#include "Socket.h"

using Symb = Printer::Symbols;

int main(int argc, char **argv) {
    std::string soc_name = {(argc != 1 ? argv[1] : "/tmp/server.socket")};

    Socket socket;

    socket.create(soc_name, Socket::FLAG_CLIENT);

    std::cout << "Epoll setup" << std::endl;

    Epoll epoll{};
    epoll.start();
    epoll.do_event(STDIN_FILENO, EPOLLIN, "Add");
    epoll.do_event(socket.data_socket, EPOLLOUT, "Add");

    std::cout << "Epoll setup completed" << std::endl;

    std::vector<std::string> requests;

    bool connected = false;

    while (true) {
        bool flag = false;
        std::unique_ptr<struct epoll_event[]> events(new struct epoll_event[BUFFER_SIZE + 1]);
        int events_size = epoll.wait(events.get());
        for (int i = 0; i < events_size; ++i) {
            struct epoll_event &event = events[i];
            if ((event.events & (EPOLLHUP | EPOLLERR)) && connected) {
                Printer::print(std::cout, "Exit", Symb::End);
                flag = true;
                break;
            } else if (event.data.fd == socket.data_socket) {
                if (event.events & EPOLLIN) {
                    Printer::print(std::cout, "Receive", Symb::End);
                    auto res = socket.read();
                    printf("Result = %s\n", res.second.get());
                } else if (event.events & EPOLLOUT) {
                    Printer::print(std::cout, "Send", Symb::End);
                    if (!connected) {
                        socket.connect();
                        connected = true;
                        epoll.do_event(socket.data_socket, EPOLLIN, "Mod");
                    } else {
                        socket.write(requests.back());
                        requests.pop_back();
                        if (requests.empty()) {
                            epoll.do_event(socket.data_socket, EPOLLIN, "Mod");
                        }
                    }
                }
            } else if (event.data.fd == STDIN_FILENO) {
                Printer::print(std::cout, "Input", Symb::End);
                std::string request;
                getline(std::cin, request);
                if (requests.empty()) {
                    epoll.do_event(socket.data_socket, EPOLLOUT | EPOLLIN | EPOLLERR | EPOLLHUP, "Mod");
                }
                requests.push_back(request);
            }
        }
        if (flag) {
            break;
        }
    }
}