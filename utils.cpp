#include <netinet/in.h>
#include <sys/socket.h>
#include "utils.h"

void utils::check(int var_to_check, const char *msg, bool soft_check) {
    if (var_to_check == -1) {
        std::perror(msg);
        if (!soft_check) exit(EXIT_FAILURE);
    }
}

void utils::send_msg(char *buf_ptr, int size, int receiver) {
    int i = 0;
    while (i < size) {
        int result = send(receiver, buf_ptr + i, size - i, MSG_NOSIGNAL);
        check(result, "in send");
        i += result;
    }
}

void utils::receive_msg(char *buf_ptr, int size, int sender) {
    int i = 0;
    while (i < size) {
        int result = recv(sender, buf_ptr + i, size - i, MSG_NOSIGNAL);
        check(result, "in receive");
        i += result;
    }
}

void utils::print_msg(const string& msg) {
    std::cout << "Received message:\n";
    std::cout << msg << std::endl;
}

bool utils::add_epoll(int epoll, int descriptor, epoll_event *ee) {
    ee->events = EPOLLIN;
    ee->data.fd = descriptor;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, descriptor, ee) == -1) {
        perror("Error while adding in epoll");
        return false;
    }
    return true;
}

void utils::handle_new_connection(int file_descriptor, int epoll_descriptor, epoll_event *ee) {
    struct sockaddr_in client_addr{};
    socklen_t address_size = sizeof(client_addr);
    int ad = accept(file_descriptor, (struct sockaddr *) &client_addr, &address_size);
    if (ad == -1) {
        perror("bad accept descriptor");
        return;
    }

    int flags = fcntl(ad, F_GETFL, 0);
    fcntl(ad, F_SETFL, flags | O_NONBLOCK);
    ee->events = EPOLLIN | EPOLLET;
    ee->data.fd = ad;
    utils::check(epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, ad, ee), "in epoll_ctl", true);
}
