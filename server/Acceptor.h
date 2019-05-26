//
// Created by vladimir on 26.05.19.
//

#ifndef OS_NET_MULTIPLEX_ACCEPTOR_H
#define OS_NET_MULTIPLEX_ACCEPTOR_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>


#include <functional>
#include <memory>

#include <Multiplexor.h>
#include <IHandle.h>

class Acceptor : IHandle {
    int fd;
    std::shared_ptr<Multiplexor> mult;

    std::function<void(void)> handler = [](){};

    char buf[1024];
    std::string message;
    size_t message_pos = 0;

public:
    Acceptor(std::shared_ptr<Multiplexor> mult, int fd);

    void handle(int ops) override { handler(); };
    void self_destroy();
    void close();
    void write();
    void read();
    void check_err();

};


#endif //OS_NET_MULTIPLEX_ACCEPTOR_H
