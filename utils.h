#ifndef OS_NET_UTILS_H
#define OS_NET_UTILS_H

#include <iostream>
#include <vector>
#include <fcntl.h>
#include <sys/epoll.h>

using std::string;

class utils {
public:
    static void check(int var_to_check, const char* msg, bool soft_check=false);
    static void receive_msg(char *buf_ptr, int size, int sender);
    static void send_msg(char *buf_ptr, int size, int receiver);
    static void print_msg(const std::vector<char>& msg);
    static bool add_epoll(int epoll, int descriptor, epoll_event* ee, int in_out, int add_mod);
    static void handle_new_connection(int file_descriptor, int epoll_descriptor, epoll_event* ee);
};


#endif //OS_NET_UTILS_H
