#ifndef OS_NET_MULTIPLEX_SERVER_H
#define OS_NET_MULTIPLEX_SERVER_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>


#include <functional>
#include <memory>

#include <Multiplexor.h>
#include <IHandle.h>

class ServerException : public std::exception {
    std::string what_;

public:
    ServerException(std::string s) : what_(s) {}

    const char * what () const throw ()
    {
        return what_.c_str();
    }
};

class Server : public IHandle {
    int socket;
    std::shared_ptr<Multiplexor> mult;
    std::function<void(void)> handler = [](){};

    bool ready = false, fail = false;
public:
    Server(std::shared_ptr<Multiplexor> mult);

    void bind(sockaddr *addr, size_t len);
    void check_err();
    void accept();

    void handle(int ops) override { handler();}

    bool isReady() { return ready; };
    bool isFailed() { return fail; }

};


#endif //OS_NET_MULTIPLEX_SERVER_H
