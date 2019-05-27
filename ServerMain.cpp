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

    std::unique_ptr<Server> srv = std::make_unique<Server>(mult);
    try {
        srv->bind((sockaddr *) &addr_server, sizeof(addr_server));
    } catch (ServerException e) {
        fprintf(stderr, "Server failed: %s\n", strerror(errno));
        return 1;
    }

    int fd, flags;
    void* ptr;

    while(!srv->isReady() && !srv->isFailed()) {
        try {
            mult->select(-1);
        } catch(MultiplexorException e) {
            fprintf(stderr, "Multiplexor retuned error: %s\n", strerror(errno));
            return 1;
        }

        try {
            while (std::get<0>(std::tie(fd, flags, ptr) = mult->next()) >= 0) {
                ((IHandle *) ptr)->handle(flags);
            }
        } catch (MultiplexorException e) {
            fprintf(stderr, "Multiplexor retuned error: %s\n", strerror(errno));
            break;
        } catch (ServerException e) {
            fprintf(stderr, "Server failed: %s\n", strerror(errno));
            break;
        }
    }


    return 0;
}
