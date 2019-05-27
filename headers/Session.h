//
// Created by utusi on 16.05.19.
//
#pragma once
#include "User.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <vector>
#include <stdio.h>
#include "Utils.h"



class Session {
public:
    Session();
    Session(int fd, User user);
    int send_msg();
    int recieve();
    void update();
    void set_user(User user);
    void set_state(States state);
    void set_data(std::string data);
    void set_fd(int fd);
    User get_user();
    std::string get_command();
    std::string get_arg();
    States get_state();
    int get_fd();
    ~Session();
private:
    States state;
    std::string command;
    std::string arg;
    std::string data;
    User user;
    int fd;
    void close_session();
};