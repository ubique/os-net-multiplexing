#ifndef NTP_SERVER_H
#define NTP_SERVER_H

#include <sys/types.h>
#include <sys/socket.h>

#include <stdint.h>
#include <time.h>

#include "protocol.h"
#include "multiplexer.h"

class NTPServer
{
public:
    explicit NTPServer(uint16_t port);
    ~NTPServer();
    [[noreturn]] void run() const;
private:
    static const int BACKLOG_SIZE = 32;

    static time_t current_ntp_time();
    static void log_connection(const struct sockaddr_storage& peer_addr);
    static void fill_packet(ntp_packet& packet);
    static void set_nonblocking(int sockfd);

    void process_client(int clientfd) const;

    int m_listenfd;
    Multiplexer m_mult;
};

#endif // NTP_SERVER_H
