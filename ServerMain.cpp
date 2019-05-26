#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#include <iostream>
#include <cstring>

#include <SelectMultiplexor.h>
#include <EpollMultiplexor.h>
#include <Server.h>

#if defined(USE_EPOLL)
using DefaultMultiplexor = EpollMultiplexor;
#else
using DefaultMultiplexor = SelectMultiplexor;
#endif


int main() {
    std::shared_ptr<Multiplexor> mult = std::make_shared<EpollMultiplexor>();

    int port = 12345;

    struct sockaddr_in addr_server;

    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = INADDR_ANY;
    addr_server.sin_port = htons(port);

    Server srv (mult);
    srv.bind((sockaddr*) &addr_server, sizeof(addr_server));

    int fd, flags;
    void* ptr;

    while(!srv.isReady() && !srv.isFailed()) {
        mult->select(-1);

        while (std::get<0>(std::tie(fd, flags, ptr) = mult->next()) >= 0) {
            ((IHandle *) ptr)->handle(flags);
        }
    }


    return 0;
}
