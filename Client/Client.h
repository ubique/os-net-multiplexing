//
// Created by Павел Пономарев on 2019-05-27.
//

#ifndef OS_NET_MULTIPLEXING_CLIENT_H
#define OS_NET_MULTIPLEXING_CLIENT_H


#include <sys/epoll.h>
#include <stdint.h>
#include <cstddef>
#include <unistd.h>
#include <string>

class Client {
public:
    explicit Client(int);
    ~Client();
    void run();
private:
    int openSocket();
    void sendAll(std::string const&);
    bool addToPoll(int, int, uint32_t);

    std::string getMessage(std::string const&);

    const static int BACKLOG = 20;
    const static size_t POLL_SIZE = 2;
    const static size_t BUFFER_SIZE = 2048;

    int mPort;
    int mEpoll;
    int mSocket;

    char responseBuffer[BUFFER_SIZE];
    struct epoll_event event;
    struct epoll_event events[POLL_SIZE];
};


#endif //OS_NET_MULTIPLEXING_CLIENT_H
