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
#include <Client.h>

#if defined(USE_EPOLL)
using DefaultMultiplexor = EpollMultiplexor;
#else
using DefaultMultiplexor = SelectMultiplexor;
#endif

int main() {
    std::shared_ptr<Multiplexor> mult = std::make_shared<DefaultMultiplexor>();

    int port = 12345;
    std::string host = "localhost";

    addrinfo hints;
    addrinfo *result;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;

    addrinfo *rp;

    int sock;

    int ret = getaddrinfo(host.c_str(), NULL, &hints, &result);

    if (ret != 0) {
        fprintf(stderr, "Could not resolve host: %s\n", gai_strerror(ret));
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        ((sockaddr_in *) rp->ai_addr)->sin_port = htons(port);
        std::unique_ptr<Client> cli;
        try {
            cli = std::make_unique<Client>(mult);
            cli->connect(rp->ai_addr, rp->ai_addrlen);
        }  catch (ClientException e) {
            fprintf(stderr, "Client failed: %s\n", strerror(errno));
            break;
        }

        int fd, flags;
        void *ptr;

        while (!cli->isReady() && !cli->isFailed()) {
            try {
                mult->select(-1);
            } catch (MultiplexorException e) {
                fprintf(stderr, "Multiplexor retuned error: %s\n", strerror(errno));
                break;
            }

            try {
                while (std::get<0>(std::tie(fd, flags, ptr) = mult->next()) >= 0) {
                    ((IHandle *) ptr)->handle(flags);
                }
            } catch (MultiplexorException e) {
                fprintf(stderr, "Multiplexor retuned error: %s\n", strerror(errno));
                break;
            } catch (ClientException e) {
                fprintf(stderr, "Client failed: %s\n", strerror(errno));
                break;
            }
        }

        if (!cli->isFailed()) break;
    }

    if (rp == NULL) {
        fprintf(stderr,
                "Connection on all addresses failed\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    return 0;
}
