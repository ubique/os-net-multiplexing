#include "client.h"

client::client(const char* addr, const char* port) : socket_client_fd(socket(AF_INET, SOCK_STREAM, 0)) {
    if (socket_client_fd.get_fd() == -1) {
        throw std::runtime_error("Can't create socket");
    }
    
    try {
        socket_addr.sin_port = std::stoul(port);
    } catch(const std::exception &e) {
        throw std::runtime_error(e.what());
    }

    socket_addr.sin_family = AF_INET;
    socket_addr.sin_addr.s_addr = inet_addr(addr);

    if (connect(socket_client_fd.get_fd(), (sockaddr*)(&socket_addr), sizeof(sockaddr_in)) == -1) {
        throw std::runtime_error("Can't connect to server");
    }
}

void client::run() {
    struct epoll_event events[MAX_EVENTS];
    struct epoll_event cur_event; 

    fd_wrapper epoll_fd_wrapper(epoll_create1(0));
    if (epoll_fd_wrapper.get_fd() == -1) {
        throw std::runtime_error("Can't create epoll");
    }

    cur_event.events = EPOLLIN;
    cur_event.data.fd = socket_client_fd.get_fd();

    if (epoll_ctl(epoll_fd_wrapper.get_fd(), EPOLL_CTL_ADD, socket_client_fd.get_fd(), &cur_event) == -1) {
        throw std::runtime_error("Epoll_ctl failed with socket_client_fd");
    }

    cur_event.data.fd = 0;
    cur_event.events = EPOLLIN;

    if (epoll_ctl(epoll_fd_wrapper.get_fd(), EPOLL_CTL_ADD, cur_event.data.fd, &cur_event) == -1) {
        throw std::runtime_error("Epoll_ctl failed with stdin");
    }
    
    bool alive = true;
    std::cout << "Request:  ";
    std::cout.flush();
    bool can_read_input = true;
    
    while (alive) {
        int cnt = epoll_wait(epoll_fd_wrapper.get_fd(), events, MAX_EVENTS, -1);
        if (cnt == -1) {
            throw std::runtime_error("Epoll_wait failed");
        }
        
        for (int i = 0; i < cnt; ++i) {
            if (events[i].data.fd == socket_client_fd.get_fd()) {
                std::vector<uint8_t> for_read_len(1);
                ssize_t read_message_length = recv(socket_client_fd.get_fd(), for_read_len.data(), 1, 0);

                if (read_message_length == -1) {
                    throw std::runtime_error("Can't read length message");
                } else if (read_message_length == 0) {
                    alive = false;
                    continue;
                }
                size_t message_length = (size_t) for_read_len[0];

                std::string response(message_length, ' ');
                size_t message_length_read = 0;

                while (message_length_read < message_length) {
                    size_t left_read = message_length - message_length_read;
                    ssize_t cur_read = recv(socket_client_fd.get_fd(), (void *) (response.data() + message_length_read), left_read, 0);
                    
                    if (cur_read == -1) {
                        throw std::runtime_error("Can't read fd");
                    } else if (cur_read == 0) {
                        alive = false;
                        error("Received incomplete message", false);
                        break;
                    }

                    message_length_read += (size_t) cur_read;
                }

                can_read_input = true;
                std::cout << "Response: " << response.data() << std::endl;
                std::cout << "Request:  ";
                std::cout.flush();
            } else if (events[i].data.fd == 0) {
                if (!can_read_input) {
                    error("Please, wait response from server", false);
                }

                std::string message;
                std::getline(std::cin, message);

                can_read_input = false;

                if (message == "-exit") {
                    return;
                } else if (message == "\n" || message == "") {
                    continue;
                } else if (message == "-help") {
                    std::cout << HELP << std::end;
                    return;
                }

                size_t message_length = message.length();
        
                if (message_length > MAX_LEN_MESSAGE) {
                    error("Too large message", false);
                    continue;
                }

                std::vector<uint8_t> for_send_len = {(uint8_t) message_length};
                if (send(socket_client_fd.get_fd(), for_send_len.data(), 1, 0) != 1) {
                    throw std::runtime_error("Can't send message");
                }

                size_t message_length_sent = 0;
                while (message_length_sent < message_length) {
                    size_t left_send = message_length - message_length_sent;
                    ssize_t cur_sent = send(socket_client_fd.get_fd(), message.c_str() + message_length_sent, left_send, 0);

                    if (cur_sent <= 0) {
                        throw std::runtime_error("Can't send message");
                    }

                    message_length_sent += (size_t) cur_sent;
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc == 2 && argv[1] != nullptr && (strcmp(argv[1], "-help") == 0)) {
        std::cerr << HELP << std::endl;
        return 0;
    } else if (argc == 2 && argv[1] != nullptr && (strcmp(argv[1], "-exit") == 0)) {
        return 0;
    }

    if (argc > 3) {
        error("Wrong usage", true, true, true);
    }

    try {
        std::string adress = "127.0.0.1";
        std::string port = "10005";

        if (argc == 2) {
            adress = argv[1];        
        }

        if (argc == 3) {
            port = argv[2];
        }

        client client(adress.c_str(), port.c_str());
        std::cout << "Echo-client: type message" << std::endl;
        
        client.run();
    } catch (std::runtime_error& e) {
        error(e.what(), true, false, true);
    }

    return 0;
}


