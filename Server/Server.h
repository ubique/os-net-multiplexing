//
// Created by Павел Пономарев on 2019-05-27.
//

#ifndef OS_NET_MULTIPLEXING_SERVER_H
#define OS_NET_MULTIPLEXING_SERVER_H

#include <sys/epoll.h>
#include <stdint.h>
#include <cstddef>
#include <unistd.h>
#include <string>

class Server {
public:
    explicit Server(int);
    ~Server();
    void run();
private:
    void process(int);
    void sendResponse(int, ssize_t);

    int openSocket();

    bool bindSocket();
    bool addToPoll(int, int, uint32_t);

    std::string getMessage(std::string const&);

    const static int BACKLOG = 20;
    const static size_t POLL_SIZE = 1024;
    const static size_t BUFFER_SIZE = 2048;

    int mPort;
    int mListener;
    int mEpoll;

    char requestBuffer[BUFFER_SIZE];
    struct epoll_event event;
    struct epoll_event events[POLL_SIZE];
};


#endif //OS_NET_MULTIPLEXING_SERVER_H
