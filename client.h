#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <netdb.h>

#include <string>
#include <stdint.h>
#include <time.h>

#include "multiplexer.h"

class NTPClient
{
public:
    NTPClient(const std::string& hostname, uint16_t port);
    ~NTPClient();
    void run() const;
private:
    static void print_help(bool error = false);

    int open_socket(struct addrinfo* addr) const;
    void send_request(bool empty) const;
    time_t receive_response() const;

    int m_socketfd;
    Multiplexer m_mult;
};

#endif // NTP_CLIENT_H
