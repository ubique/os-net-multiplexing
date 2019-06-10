//
// Created by Yaroslav on 08/06/2019.
//

#ifndef TASK5_UTILS_H
#define TASK5_UTILS_H

#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>


int const SERVER_PORT = 8089;
void fun_send(char *what, int amount, int where, std::string message);
void fun_recv(char *where, int amount, int from, std::string message);
void fun_connect(int file_descriptor, int epoll_descriptor, epoll_event* ee);

void add_new_client(const int* master, sockaddr_in* client, int* poll_s, socklen_t* size_s);
void is_failure(int rc, const std::string &problem_name);

#endif //TASK5_UTILS_H
