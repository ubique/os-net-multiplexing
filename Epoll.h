//
// Created by max on 07.05.19.
//
#pragma once

#include <unistd.h>
#include <cstdint>
#include <sys/epoll.h>
#include <string>

class Epoll {
public:

    Epoll();

    void start();

    void close();

    void close(int);

    void do_event(int, uint32_t, const std::string &);

    int wait(struct epoll_event *events);

    int fd;

    ~Epoll();
};


