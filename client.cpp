#include "client.h"

client::client(const char *addr, const char *port) : socket_client_fd(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) {
    if (socket_client_fd.get_fd() == -1) {
        throw std::runtime_error("Can't create socket");
    }

    try {
        socket_addr.sin_port = std::stoul(port);
    } catch (const std::exception &e) {
        throw std::runtime_error(e.what());
    }

    socket_addr.sin_family = AF_INET;
    socket_addr.sin_addr.s_addr = inet_addr(addr);
}

void prompt(bool &can_read_input) {
    std::cout << "Request:  ";
    std::cout.flush();
    can_read_input = true;
}

struct base_data {
    bool have_length;
    size_t og_length;
    size_t pr_length;
    std::string message;

    base_data() :
            have_length(false),
            og_length(0),
            pr_length(0),
            message("") {}

    base_data(const std::string &message) :
            have_length(false),
            og_length(message.length()),
            pr_length(0),
            message(message) {}
};

struct client_data {
    base_data for_read;
    base_data for_send;
};

void new_message(const std::string &str, client_data &cur_data) {
    cur_data.for_send = {str};
    cur_data.for_read = {""};
}

void client::run() {
    struct epoll_event events[MAX_EVENTS];
    struct epoll_event cur_event{};

    fd_wrapper epoll_fd_wrapper(epoll_create1(0));
    if (epoll_fd_wrapper.get_fd() == -1) {
        throw std::runtime_error("Can't create epoll");
    }

    size_t fd = socket_client_fd.get_fd();

    if (connect(fd, (sockaddr *) (&socket_addr), sizeof(sockaddr_in)) == -1 && errno != EINPROGRESS) {
        throw std::runtime_error("Can't connect to server");
    }

    cur_event.events = EPOLLOUT;
    cur_event.data.fd = fd;

    if (epoll_ctl(epoll_fd_wrapper.get_fd(), EPOLL_CTL_ADD, fd, &cur_event) == -1) {
        throw std::runtime_error("Epoll_ctl failed with socket_client_fd");
    }

    bool have_connected = false;
    while (!have_connected) {
        int cnt = epoll_wait(epoll_fd_wrapper.get_fd(), events, 1, -1);
        if (cnt == -1) {
            throw std::runtime_error("Epoll_wait failed");
        }
        if (events[0].events & EPOLLOUT) {
            int connect_error = 0;

            socklen_t err_len = sizeof(connect_error);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &connect_error, &err_len) == -1) {
                throw std::runtime_error("Getsockopt failed");
            }

            if (connect_error != 0) {
                throw std::runtime_error("Connect failed");
            }

            have_connected = true;
        }
    }

    cur_event.data.fd = 0;
    cur_event.events = EPOLLIN;

    if (epoll_ctl(epoll_fd_wrapper.get_fd(), EPOLL_CTL_ADD, cur_event.data.fd, &cur_event) == -1) {
        throw std::runtime_error("Epoll_ctl failed with stdin");
    }

    bool alive = true;
    bool can_read_input;
    prompt(can_read_input);

    client_data cur = {};

    while (alive) {
        int cnt = epoll_wait(epoll_fd_wrapper.get_fd(), events, MAX_EVENTS, -1);
        if (cnt == -1) {
            throw std::runtime_error("Epoll_wait failed");
        }

        for (int i = 0; i < cnt; ++i) {
            if (events[i].data.fd == fd) {
                bool want_set_in = false;

                if (events[i].events & EPOLLOUT) {
                    if (!cur.for_send.have_length && cur.for_send.og_length > 0) {
                        std::vector<uint8_t> for_send_len = {(uint8_t) cur.for_send.og_length};
                        if (send(fd, for_send_len.data(), 1, MSG_NOSIGNAL) != 1) {
                            throw std::runtime_error("Can't send message");
                        }

                        cur.for_send.have_length = true;
                    } else if (cur.for_send.pr_length < cur.for_send.og_length) {
                        size_t left_send = cur.for_send.og_length - cur.for_send.pr_length;
                        ssize_t cur_sent = send(fd, cur.for_send.message.c_str() + cur.for_send.pr_length, left_send,
                                                MSG_NOSIGNAL);

                        if (cur_sent <= 0) {
                            throw std::runtime_error("Can't send message");
                        }

                        cur.for_send.pr_length += (size_t) cur_sent;
                    }
                    want_set_in = true;
                }
                if (events[i].events & EPOLLIN) {
                    if (!cur.for_read.have_length) {
                        std::vector<uint8_t> for_read_len(1);
                        ssize_t read_message_length = recv(fd, for_read_len.data(), 1, 0);

                        if (read_message_length <= 0) {
                            throw std::runtime_error("Can't read length message");
                        }

                        cur.for_read.have_length = true;
                        cur.for_read.og_length = for_read_len[0];
                        cur.for_read.message.resize(for_read_len[0]);
                    } else {
                        size_t left_read = cur.for_read.og_length - cur.for_read.pr_length;
                        if (left_read > 0) {
                            ssize_t part_read = recv(fd, (void *) (cur.for_read.message.data() + cur.for_read.pr_length), left_read, 0);

                            if (part_read == -1) {
                                throw std::runtime_error("Can't read fd");
                            } else if (part_read == 0) {
                                alive = false;
                                error("Received incomplete message");
                            } else {
                                cur.for_read.pr_length += part_read;
                            }
                        }

                        if (cur.for_read.pr_length == std::min(cur.for_send.pr_length, cur.for_read.og_length)) {
                            events[i].events = EPOLLOUT;
                            epoll_ctl(epoll_fd_wrapper.get_fd(), EPOLL_CTL_MOD, fd, &events[i]);
                            want_set_in = false;
                        }

                        if (cur.for_read.pr_length == cur.for_read.og_length) {
                            std::cout << "Response: " << cur.for_read.message << std::endl;
                            new_message("", cur);

                            if (alive) {
                                prompt(can_read_input);
                            }
                        }
                    }
                }

                if (want_set_in) {
                    events[i].events |= EPOLLIN;
                    epoll_ctl(epoll_fd_wrapper.get_fd(), EPOLL_CTL_MOD, fd, &events[i]);
                }
            } else if (events[i].data.fd == 0) {
                std::string input_message;
                std::getline(std::cin, input_message);

                if (input_message == "-exit") {
                    return;
                } else if (input_message == "\n" || input_message.empty()) {
                    prompt(can_read_input);
                    continue;
                } else if (input_message == "-help") {
                    std::cout << HELP << std::endl;
                    return;
                }

                if (!can_read_input) {
                    std::cout << "Please, wait response from server" << std::endl;
                    continue;
                }

                if (input_message.length() > MAX_LEN_MESSAGE) {
                    error("Too large message", false);
                    prompt(can_read_input);
                    continue;
                }

                new_message(input_message, cur);
                can_read_input = false;
            }
        }
    }
}

int main(int argc, char *argv[]) {
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
    } catch (std::runtime_error &e) {
        std::cout << std::endl;
        error(e.what(), true, false, true);
    }

    return 0;
}


