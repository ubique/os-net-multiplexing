#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
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
        int result = write(receiver, buf_ptr + i, size - i);
        check(result, "in send");
        i += result;
    }
}

void utils::receive_msg(char *buf_ptr, int size, int sender) {
    int i = 0;
    while (i < size) {
        int result = read(sender, buf_ptr + i, size - i);
        check(result, "in receive");
        i += result;
    }
}

void utils::print_msg(const std::vector<char>& msg) {
    std::cout << "Received message:\n";
    for (const char& x: msg) {
        std::cout << x;
    }
    std::cout << std::endl;
}

bool utils::add_epoll(int epoll, int descriptor, epoll_event *ee, int in_out, int add_mod) {
    ee->events = in_out;
    ee->data.fd = descriptor;
    if (epoll_ctl(epoll, add_mod, descriptor, ee) == -1) {
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
    utils::add_epoll(epoll_descriptor, ad, ee, EPOLLIN, EPOLL_CTL_ADD);
}
