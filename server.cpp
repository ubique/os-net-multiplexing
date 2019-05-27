#include "server.h"

server::server(const char* address, const char* port) : socket_fd(socket(AF_INET, SOCK_STREAM, 0)) {
    if (socket_fd.get_fd() == -1) {
        throw std::runtime_error("Can't create socket");
    }

    memset(&socket_addr, 0, sizeof(sockaddr_in));

    try {
        socket_addr.sin_port = (uint16_t)(std::stoul(port));
    } catch(const std::exception &e) {
        throw std::runtime_error(e.what());
    }
    
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_addr.s_addr = inet_addr(address);

    if (bind(socket_fd.get_fd(), (sockaddr*)(&socket_addr), sizeof(sockaddr)) == -1) {
        throw std::runtime_error("Can't bind socket");
    }

    if (listen(socket_fd.get_fd(), 5) == -1) {
        throw std::runtime_error("Can't listen socket");
    }
}

void server::run() {
    std::cout << "Server is running..." << std::endl;
    struct epoll_event events[EVENTS];
    struct epoll_event cur_event; 

    fd_wrapper epoll_fd_wrapper(epoll_create1(0));
    if (epoll_fd_wrapper.get_fd() == -1) {
        error("Can't create epoll");
    }

    cur_event.data.fd = socket_fd.get_fd();
    cur_event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd_wrapper.get_fd(), EPOLL_CTL_ADD, socket_fd.get_fd(), &cur_event) == -1) {
        throw std::runtime_error("Epoll_ctl failed with socket_fd");
    }

    while (true) {
        int cnt = epoll_wait(epoll_fd_wrapper.get_fd(), events, EVENTS, -1);
        if (cnt == -1) {
            throw std::runtime_error("Epoll_waiting failed");
        }
        
        for (int i = 0; i < cnt; ++i) {
            if (events[i].data.fd == socket_fd.get_fd()) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof(struct sockaddr_in);;
                int client_fd(accept(socket_fd.get_fd(), (sockaddr*)(&client_addr), &client_addr_size));
                if (client_fd == -1) {
                    error("Can't accept client address");
                    continue;
                }

                std::cout << "Client connected" << std::endl;

                fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);
                cur_event.events = EPOLLIN | EPOLLET;
                cur_event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd_wrapper.get_fd(), EPOLL_CTL_ADD, client_fd, &cur_event) == -1) {
                    error("Can't add client to epoll_ctl");

                    if (client_fd != -1) {
                        if (close(client_fd) == -1) {
                            std::cerr << "Can't close fd" << std::endl;
                        }
                    }
                }
            } else {
                char buffer[BUF_SIZE];

                int client_fd(events[i].data.fd);
                ssize_t read = recv(client_fd, buffer, BUF_SIZE, 0);
                
                if (read > 0) {
                    if (write(client_fd, buffer, (size_t)(read)) == -1) {
                        error("Can't send message");
                    }
                } else {
                    if (read == -1) {
                        error("Can't receive");
                    } else {
                        std::cout << "Client disconnected" << std::endl;
                        //error("Client disconnected", false);
                    }

                    if (epoll_ctl(epoll_fd_wrapper.get_fd(), EPOLL_CTL_DEL, client_fd, nullptr) == -1) {
                        if (client_fd != -1) {
                            if (close(client_fd) == -1) {
                                std::cerr << "Can't close fd" << std::endl;
                            }
                        }
                    }
                } 
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc == 2 && argv[1] != nullptr && argv[1] == "-help") {
        std::cerr << HELP << std::endl;
        return 0;
    }
    if (argc > 3) {
        error("Wrong usage", false, true, true);
    }

    try {
        std::string address = "127.0.0.1";
        std::string port = "10005";

        if (argc == 2) {
            address = argv[1];        
        }

        if (argc == 3) {
            port = argv[2];
        }

        server server(address.c_str(), port.c_str());
        server.run();
    } catch (const std::invalid_argument &e) {
        error(e.what(), true, true, true);
    } catch (const std::runtime_error& e) {
        error(e.what(), true, false, true);
    }
}


