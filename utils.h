//
// Created by Yaroslav on 08/06/2019.
//

#ifndef TASK5_UTILS_H
#define TASK5_UTILS_H

#include <string>
#include <sys/epoll.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>


using std::string;
using std::cout;

int const SERVER_PORT = 8080;
const size_t BUF_SIZE = 100;
const size_t MAX_EPOLL_EVENTS = 16;
void check_error(int rc, const std::string& additional);
void check_non_stop(int rc, const std::string& additional);
void fun_send(int fd, char* what, int size, string message);
void fun_recv(int fd, char* what, string message);

bool fun_send_client(int fd, char* what, int size, string message);
bool fun_recv_client(int fd, char* what, string message);


#endif //TASK5_UTILS_H
