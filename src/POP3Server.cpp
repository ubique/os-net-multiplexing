#include <iostream>
#include <Session.h>
#include <sys/epoll.h>
#include "POP3Server.h"
#include "Utils.h"
#include "DBase.h"

POP3Server::POP3Server(const std::string &host_name, int port = 110) {
    struct hostent *server;

    socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socket_fd < 0) {
        print_error("ERROR opening socket");
    }

    server = gethostbyname(host_name.c_str());
    if (server == nullptr) {
        print_error("ERROR, no such host");
        stop();
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(reinterpret_cast<char*>(&server_addr.sin_addr.s_addr), static_cast<char*>(server->h_addr), server->h_length);
    server_addr.sin_port = htons(port);

    if (bind(socket_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        print_error("ERROR bind a socket");
        stop();
    }

    if (listen(socket_fd, COUNT_OF_CLIENTS) == -1) {
        print_error("ERROR listen socket");
        stop();
    }
}

void POP3Server::print_error(const std::string &msg) {
    std::cerr << msg << " " << strerror(errno) << std::endl;
}


int POP3Server::epoll_ctl_wrap(int epool_fd, int op, int fd, uint32_t events) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(epool_fd, op, fd, &event) == -1) {
        print_error("Can't make epoll_ctl");
        return -1;
    }
    return 0;
}

int POP3Server::delete_socket(int epoll, int fd, std::map<int, Session> &sessions) {
    if (epoll_ctl_wrap(epoll, EPOLL_CTL_DEL, fd, 0) == -1) {
        print_error("Can't delete socket from poll");
        return -1;
    }
    sessions.erase(fd);
    return 0;
}

void debug(std::string s) {
    std::cout << "Debug: " << s << std::endl;
}

int POP3Server::run() {
    int epoll_fd = epoll_create(COUNT_OF_CLIENTS);
    if (epoll_fd == -1) {
        print_error("Can't create a epoll");
        exit(EXIT_FAILURE);
    }
    if (epoll_ctl_wrap(epoll_fd, EPOLL_CTL_ADD, socket_fd, EPOLLIN) == -1) {
        print_error("Can't add a server socket in pool");
        exit(EXIT_FAILURE);
    }
    DBase data_base;
    std::map<int, Session> sessions;
    struct epoll_event events[COUNT_OF_EVENTS];
    while (true) {
        int count = epoll_wait(epoll_fd, events, COUNT_OF_EVENTS, -1);
        debug("count " + std::to_string(count));
        if (count == -1) {
            debug("fail count");
            if (errno != EINTR) {
                print_error("Can't error in waiting");
                break;
            }
            continue;
        }
        debug("loop");
        for(size_t i = 0; i < count; i++) {
            if (events[i].data.fd == socket_fd) {
                debug("in server server");
                int client_fd = accept(socket_fd, nullptr, nullptr);
                debug(std::to_string(client_fd));
                if (client_fd == -1) {
                    if (errno !=EAGAIN && errno != EINTR) {
                        print_error("Can't accept a client!");
                    }
                    continue;
                }
                if (epoll_ctl_wrap(epoll_fd, EPOLL_CTL_ADD, client_fd, EPOLLIN | EPOLLOUT | EPOLLERR) == -1) {
                    print_error("Can't add client socket in poll");
                    close(client_fd);
                } else {
                    debug("create_session");
                    Session session;
                    session.set_fd(client_fd);
                    sessions[client_fd] = session;
                    sessions[client_fd].set_data("Server ready!");
                }
            } else {
                debug("in server client");
                int client_fd = events[i].data.fd;
                if (events[i].events & (EPOLLHUP | EPOLLERR)) {
                    print_error("Error event");
                    delete_socket(epoll_fd, client_fd, sessions);
                    continue;
                }
                if (events[i].events & EPOLLIN) {
                    if (sessions[client_fd].recieve() == -1) {
                        delete_socket(epoll_fd, client_fd, sessions);
                        continue;
                    }
                    std::cout << "state: " << sessions[client_fd].get_state() << std::endl;
                    std::cout << sessions[client_fd].get_command() << std::endl;
                    if (sessions[client_fd].get_state() == AUTHORIZATION) {
                        if (sessions[client_fd].get_command() == "USER") {
                            if (!sessions[client_fd].get_arg().empty()) {
                                if (data_base.is_user_by_login(sessions[client_fd].get_arg())) {
                                    sessions[client_fd].set_data("+OK, user was found");
                                    sessions[client_fd].set_user(data_base.get_user_by_login(sessions[client_fd].get_arg()));
                                } else {
                                    sessions[client_fd].set_data("-ERR, can't find user");
                                }
                            } else {
                                sessions[client_fd].set_data("-ERR no login");
                            }
                        } else if (sessions[client_fd].get_command() == "PASS") {
                            if (!sessions[client_fd].get_arg().empty()) {
                                if (sessions[client_fd].get_user().get_login().empty()) {
                                    sessions[client_fd].set_data("-ERR, haven't user session");
                                } else if (sessions[client_fd].get_user().cmp_password(sessions[client_fd].get_arg())) {
                                    sessions[client_fd].set_data("+OK, authorization success");
                                    sessions[client_fd].set_state(TRANSACTION);
                                } else {
                                    sessions[client_fd].set_data("-ERR, password is incorrect");
                                }
                            } else {
                                sessions[client_fd].set_data("-ERR no password");
                            }
                        } else if (sessions[client_fd].get_command() == "QUIT") {
                            sessions[client_fd].set_data("+OK, session closed");
                            delete_socket(epoll_fd, client_fd, sessions);
                        } else {
                            sessions[client_fd].set_data("-ERR, unknown command. You must authorization");
                        }
                    } else {
                        if (sessions[client_fd].get_command() == "QUIT") {
                            sessions[client_fd].set_data("+OK, all messages updated");
                            data_base.update(sessions[client_fd].get_user().get_login());
                            delete_socket(epoll_fd, client_fd, sessions);
                            close(client_fd);
                        } else if (sessions[client_fd].get_command() == "STAT") {
                            auto messages = data_base.get_messages_by_login(sessions[client_fd].get_user().get_login());
                            std::string response = "+OK " + std::to_string(messages.size()) + " " +
                                                   std::to_string(get_size_of_vector(messages));
                            sessions[client_fd].set_data(response);
                        } else if (sessions[client_fd].get_command() == "LIST") {
                            auto messages = data_base.get_messages_by_login(sessions[client_fd].get_user().get_login());
                            std::string response;
                            if (messages.empty()) {
                                response = "-ERR, no such message";
                            } else {
                                response = "+OK " + std::to_string(messages.size()) + " messages (" +
                                           std::to_string(get_size_of_vector(messages)) + "bytes)" + CRLF;
                                if (!sessions[client_fd].get_arg().empty()) {
                                    size_t ind = std::stoi(sessions[client_fd].get_arg());
                                    if (ind <= 0 || ind > messages.size()) {
                                        response = "-ERR, incorrect number of messages";
                                    } else {
                                        response += sessions[client_fd].get_arg() + " " + std::to_string(messages[ind - 1].get_size());
                                    }
                                } else {
                                    for (size_t i = 0; i < messages.size(); i++) {
                                        response += std::to_string(i + 1) + " " + std::to_string(messages[i].get_size()) +
                                                    (messages.size() - 1 == i ? "" : CRLF);
                                    }
                                }
                            }
                            sessions[client_fd].set_data(response);
                        } else if (sessions[client_fd].get_command() == "RETR") {
                            if (!sessions[client_fd].get_arg().empty()) {
                                auto messages = data_base.get_messages_by_login(sessions[client_fd].get_user().get_login());
                                std::string response;
                                if (messages.empty()) {
                                    response = "-ERR no such elements";
                                } else {
                                    size_t ind = std::stoi(sessions[client_fd].get_arg());
                                    if (ind <= 0 || messages.size() < ind) {
                                        response = "-ERR incorrect format of number";
                                    } else {
                                        response = "+OK " + std::to_string(messages[ind - 1].get_size()) + " bytes" + CRLF;
                                        response += messages[ind - 1].get_text() + CRLF;
                                    }
                                }
                                sessions[client_fd].set_data(response);
                            } else {
                                sessions[client_fd].set_data("-ERR");
                            }
                        } else if (sessions[client_fd].get_command() == "DELE") {
                            if (!sessions[client_fd].get_arg().empty()) {
                                auto messages = data_base.get_messages_by_login(sessions[client_fd].get_user().get_login());
                                size_t ind = std::stoi(sessions[client_fd].get_arg());
                                std::string response;
                                if (ind <= 0 || messages.size() < ind) {
                                    response = "-ERR incorrect format of number";
                                } else {
                                    if (messages[ind - 1].is_removed()) {
                                        response = "-ERR alredy deleted";
                                    } else {
                                        data_base.remove_msg(sessions[client_fd].get_user().get_login(), ind - 1);
                                        response = "+OK message deleted";
                                    }
                                }
                                sessions[client_fd].set_data(response);
                            } else {
                                sessions[client_fd].set_data("-ERR haven't number");
                            }
                        } else if (sessions[client_fd].get_command() == "NOOP") {
                            sessions[client_fd].set_data("+OK");
                        } else if (sessions[client_fd].get_command() == "RSET") {
                            data_base.unremove_msgs(sessions[client_fd].get_user().get_login());
                            sessions[client_fd].set_data("+OK");
                        } else {
                            sessions[client_fd].set_data("-ERR, unknown command");
                        }
                    }
                    epoll_ctl_wrap(epoll_fd, EPOLL_CTL_MOD, client_fd, EPOLLOUT);
                }
                if (events[i].events & EPOLLOUT) {
                    sessions[client_fd].send_msg();
                    epoll_ctl_wrap(epoll_fd, EPOLL_CTL_ADD, client_fd, EPOLLIN | EPOLLHUP | EPOLLERR);
                }
            }
        }
    }
    return 0;
}

POP3Server::~POP3Server() {
    stop();
}

size_t POP3Server::get_size_of_vector(std::vector<Message> &messages) {
    size_t res = 0;
    for(Message msg : messages) {
        res += msg.get_size();
    }
    return res;
}

void POP3Server::stop() {
    if (close(socket_fd) == -1) {
        print_error("Can't stop a server");
    }
    // some operations
}
