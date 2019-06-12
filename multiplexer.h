#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include <vector>

class Multiplexer
{
public:
    enum evtype {POLLIN = 0x1, POLLOUT = 0x2, POLLET = 0x4};
    struct event {
        event() : fd(-1), type(0) {}
        int fd, type;
    };
    static const int MAX_EVENTS = 1024;

    Multiplexer();
    ~Multiplexer();

    void add_polled(int fd, int flags = POLLIN) const;
    void change_polled(int fd, int flags = POLLIN) const;
    bool delete_polled(int fd) const;
    std::vector<event> get_ready() const;
private:
    int m_pollfd;
};

#endif // MULTIPLEXER_H
