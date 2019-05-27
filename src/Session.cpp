#include <utility>

#include <utility>

//
// Created by utusi on 16.05.19.
//

#include "Session.h"
#include <ctime>
#include <unistd.h>
#include <iostream>
#include <Utils.h>

void Session::update() {
    // update time
}

User Session::get_user() {
    return this->user;
}

Session::Session() : user(), state(AUTHORIZATION), data("") {}

Session::Session(int fd, User user) : user(std::move(user)), fd(0), state(AUTHORIZATION), data("") {}

int Session::send_msg() {
    if (send(fd, ((data.empty() ? "Default msg" : "data") + CRLF).c_str(), std::max(data.size(), (size_t)11) + 1, 0) == -1) {
        std::cerr << "Can't send a msg for" + user.get_login() << std::endl;
        close_session();
    }
}

int Session::recieve() {
    const size_t BUFFER_LENGHT = 1024;
    char buffer[BUFFER_LENGHT];
    memset(buffer, 0, BUFFER_LENGHT);
    size_t msg_len = recv(fd, &buffer, BUFFER_LENGHT, 0);
    if (msg_len == -1) {
        std::cerr << "Error to receive a message" << std::endl;
        this->command = "";
        this->arg = "";
        return -1;
    }
    std::vector<std::string> param = Utils::split(buffer);
    this->command = param[0];
    if (param.size() == 2) {
        this->command = param[1];
    }
    return 0;
}

void Session::set_user(User user) {
    this->user = std::move(user);
}

Session::~Session() {
    close_session();
}

void Session::close_session() {
    if (close(fd) == -1) {
        std::cerr << "Can't close a session with user" + user.get_login() << std::endl;
    }
}

States Session::get_state() {
    return this->state;
}

void Session::set_state(States state) {
    this->state = state;
}

int Session::get_fd() {
    return this->fd;
}

void Session::set_fd(int fd) {
    this->fd = fd;
}


std::string Session::get_command() {
    return command;
}

std::string Session::get_arg() {
    return arg;
}

void Session::set_data(std::string data) {
    this->data = data;
}

