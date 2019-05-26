#ifndef OS_NET_MULTIPLEX_CLIENT_H
#define OS_NET_MULTIPLEX_CLIENT_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>


#include <functional>
#include <memory>

#include <Multiplexor.h>
#include <IHandle.h>

class ClientException : public std::exception {
    std::string what_;

public:
    ClientException(std::string s) : what_(s) {}

    const char * what () const throw ()
    {
        return what_.c_str();
    }
};

class Client : public IHandle {
    int socket;
    std::shared_ptr<Multiplexor> mult;
    std::function<void(void)> handler = [](){};

    std::string message = "Hello\n";
    size_t message_pos = 0;

    char buf[1024];
    std::string response;

    bool ready = false, fail = false;
public:
    Client(std::shared_ptr<Multiplexor> mult);

    void connect(sockaddr *addr, size_t len);
    void check_err();
    void write();
    void read();
    void close();

    void handle(int ops) override { handler();}

    bool isReady() { return ready; };
    bool isFailed() { return fail; }

};


#endif //OS_NET_MULTIPLEX_CLIENT_H
