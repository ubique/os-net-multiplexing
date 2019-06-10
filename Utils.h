//
// Created by utusi on 10.06.19.
//

#pragma once

#include <vector>
#ifdef __linux
#include <sys/epoll.h>
#include <sys/ioctl.h>

#endif
#ifdef __APPLE__
#include <sys/event.h>
#endif

const int CNT_EVENTS = 32;
const int CNT_USERS = 32;
const int SIZE_SEND_BUF = 1024;
const int SIZE_READ_BUF = 1024;


/*#ifdef __linux
    struct epoll_event events[CNT_EVENTS];
#elif __APPLE__
    struct kevent events[EVENT_BUFF_SIZE];
#endif*/

void print_error(const std::string& s) {
    std::cerr << s << " Error: " << strerror(errno) << std::endl;
}

int set_non_blocking(int fd) {
    int flags;
#if  defined(O_NONBLOCK)
    if ((flags = fcntl(fd, F_GETFL, 0) == -1)) {
        flags = 0;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIONBIO, &flags);
#endif
}

int create() {
    int fd = -1;
    #ifdef __linux
        fd = epoll_create1(0);
        if (fd == -1) {
            print_error("Can't create poll");
        }
    #elif __APPLE__
        fd = kqueue();
        if (fd == -1) {
            print_error("Can't create kqueue");
        }
    #endif
    return fd;
}

int wait(std::vector<int> &fds, int poll) {
    int size = 0;
    #ifdef __linux
        //size = epoll_wait(poll, events, CNT_EVENTS, -1);
    #elif __APPLE__
        size = kevent(poll, NULL, 0, CNT_EVENTS, NULL);
    #endif
    fds.assign(size, 0);
    for (int i = 0; i < size; i++) {
        //fds[i] = events[i].data.fd;
    }
    return size;
}

int epoll_ctl_wrap(int poll, int op, int fd, uint32_t events) {
        struct epoll_event event{};
        event.data.fd = fd;
        event.events = events;
       return epoll_ctl(poll, op, fd, &event) == -1;
}

int delete_socket(int epoll, int fd) {
    return epoll_ctl_wrap(epoll, EPOLL_CTL_DEL, fd, 0) == -1;
}

int read(int fd, std::string &s) {
    s.clear();
    char buf[SIZE_READ_BUF + 1];
    memset(buf, 0, SIZE_READ_BUF + 1);
    int count = 0;
    while(true) {
        count = recv(fd, buf, SIZE_READ_BUF, MSG_NOSIGNAL);
        if (count == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                return 0;
            }
            print_error("here");
            return -1;
        }
        if (count == 0 && errno == EAGAIN) {
            return 0;
        }
        for(int i = 0; i < count; i++) {
            s.push_back(buf[i]);
        }
    }
}

int sendall(int s, char *buf, int len, int flags) {
    int total = 0;
    int size = 0;

    while(total < len) {
        size = send(s, buf + total, len - total, flags);
        if(size == -1) {
            break;
        }
        total += size;
    }
    return size == -1 ? -1 : total;
}

int write(int fd, std::string &s) {
    char buf[SIZE_SEND_BUF + 1];
    int len = std::min(SIZE_SEND_BUF, (int)s.size());
    strncpy(buf, s.c_str(), len);
    return sendall(fd, buf, len, 0);
}

void log(std::string s) {
    std::cerr << "Log: " << s << std::endl;
}