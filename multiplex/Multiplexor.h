#ifndef OS_NET_MULTIPLEX_MULTIPLEXOR_H
#define OS_NET_MULTIPLEX_MULTIPLEXOR_H


#include <exception>
#include <string>


class MultiplexorException : public std::exception {
    std::string what_;

public:
    MultiplexorException(std::string s) : what_(s) {}

    const char * what () const throw ()
    {
        return what_.c_str();
    }
};

class Multiplexor {
public:
    enum {
        OP_READ = 1,
        OP_WRITE = 2,
        OP_EXCEPT = 4
    };

    virtual ~Multiplexor() = default;
    virtual void add_fd(int fd, int ops, void* attachment) = 0;
    virtual void remove_fd(int fd) = 0;
    virtual int select(int timeout_ms) = 0;
    virtual std::tuple<int, int, void*> next() = 0;



};


#endif //OS_NET_MULTIPLEX_MULTIPLEXOR_H
