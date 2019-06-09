#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include <vector>

class Multiplexer
{
public:
    Multiplexer();
    ~Multiplexer();

    void add_polled(int fd, bool edge_triggered = false) const;
    bool delete_polled(int fd) const;
    std::vector<int> get_ready() const;
private:
    static const int MAX_EVENTS = 1024;

    int m_pollfd;
};

#endif // MULTIPLEXER_H
