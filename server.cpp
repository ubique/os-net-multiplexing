#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <string>
#include <chrono>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <system_error>

#include "server.h"
#include "protocol.h"

NTPServer::NTPServer(uint16_t port)
{
    struct addrinfo hints = {};
    hints.ai_family    = AF_INET6;    // Explicitly specify IPv6
                                      // By doing so and relying on IPV6_V6ONLY,
                                      // we are able to accept both IPv4 & IPv6
    hints.ai_socktype  = SOCK_STREAM; // Stream socket
    hints.ai_flags     = AI_PASSIVE;  // For wildcard IP address
    hints.ai_protocol  = IPPROTO_TCP;
    hints.ai_canonname = nullptr;
    hints.ai_addr      = nullptr;
    hints.ai_next      = nullptr;

    struct addrinfo* server_addr;
    int s = getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &server_addr);
    if (s != 0) {
        throw std::runtime_error(std::string("getaddrinfo failed: ") + gai_strerror(s));
    }

    // getaddrinfo() returns a list of address structures
    // Trying each until we successfully bind
    struct addrinfo* rp;
    for (rp = server_addr; rp != nullptr; rp = rp->ai_next) {
        m_listenfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        // Attempting to disable IPV6_V6ONLY; if this fails, we can still work
        // with IPv6 clients. Depends on OS: on Linux is default, not on BSDs.
        const int NO = 0;
        if (setsockopt(m_listenfd, IPPROTO_IPV6, IPV6_V6ONLY, &NO, sizeof(NO)) == -1) {
            std::cerr << "Failed to disable IPV6_V6ONLY: " << strerror(errno) << std::endl;
        }
        if (m_listenfd != -1) {
            if (bind(m_listenfd, rp->ai_addr, rp->ai_addrlen) == 0) {
                break;  // Success
            }
            close(m_listenfd);
            m_listenfd = -1;
        }
    }
    freeaddrinfo(server_addr);

    // All addresses failed
    if (rp == nullptr) {
        std::error_code ec(errno, std::system_category());
        throw std::system_error(ec, "Failed to bind the socket");
    }

    if (listen(m_listenfd, BACKLOG_SIZE) == -1) {
        std::error_code ec(errno, std::system_category());
        throw std::system_error(ec, "Failed to listen on socket");
    }

    m_mult.add_polled(m_listenfd);
}

NTPServer::~NTPServer()
{
    if (close(m_listenfd) == -1) {
        std::cerr << "Failed to close the socket: " << strerror(errno) << std::endl;
    }
}

[[noreturn]] void NTPServer::run() const
{
    for (;;) {
        auto ready = m_mult.get_ready();

        for (int fd : ready) {
            if (fd == m_listenfd) {
                struct sockaddr_storage addr = {};
                socklen_t addrlen = sizeof(addr);
                int client = accept(m_listenfd, reinterpret_cast<struct sockaddr*>(&addr), &addrlen);
                if (client == -1) {
                    perror("Connection failed");
                    std::cerr << "Connection to new client failed: " << strerror(errno) << std::endl;
                    continue;
                }

                set_nonblocking(client);

                try {
                    m_mult.add_polled(client, true);
                } catch (std::runtime_error& e) {
                    std::cerr << "Error adding client: " << e.what() << std::endl;
                    if (close(client) == -1) {
                        std::cerr << "Failed to close client fd: " << strerror(errno) << std::endl;
                    }
                }

                log_connection(addr);
            } else {
                process_client(fd);
            }
        }
    }
}

time_t NTPServer::current_ntp_time()
{
    auto sys_time = std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(sys_time) + NTP_TIMESTAMP_DELTA;
}

void NTPServer::log_connection(const sockaddr_storage &peer_addr)
{
    char host[NI_MAXHOST], service[NI_MAXSERV];
    int s = getnameinfo(reinterpret_cast<const struct sockaddr *>(&peer_addr),
                    sizeof(struct sockaddr_storage), host, NI_MAXHOST,
                    service, NI_MAXSERV, NI_NUMERICSERV);
    if (s == 0) {
        std::cout << "Connection from " << host << ":" << service << std::endl;
    } else {
        std::cerr << "getaddrinfo failed: " << gai_strerror(s) << std::endl;
    }
}

void NTPServer::fill_packet(ntp_packet &packet)
{
    packet.li_vn_mode = 0b11'011'100; // li = 3 (unknown), vn = 3 (version), mode = 4 (server)
    packet.stratum    = 0b00001111;   // class 15; embracing our imperfections.
    // Only setting seconds from a system clock
    packet.txTm_s = htonl(static_cast<uint32_t>(NTPServer::current_ntp_time()));
    packet.txTm_f = 0;
}

void NTPServer::set_nonblocking(int sockfd)
{
    int options = fcntl(sockfd, F_GETFL);

    if (options == -1) {
        std::error_code ec(errno, std::system_category());
        throw std::system_error(ec, "Failed to get socket options");
    }

    options |= O_NONBLOCK;

    if (fcntl(sockfd, F_SETFL, options) < 0) {
        std::error_code ec(errno, std::system_category());
        throw std::system_error(ec, "Failed to make socket non-blocking");
    }
}

void NTPServer::process_client(int clientfd) const
{
    ntp_packet packet = {};

    // Receive client request
    ssize_t ntransferred = read(clientfd, &packet, sizeof(packet));
    if (ntransferred == sizeof (packet)) {
        fill_packet(packet);

        // Send resulting packet
        ntransferred = write(clientfd, &packet, sizeof(packet));

        if (ntransferred == -1) {
            std::cerr << "Error sending response: " << strerror(errno) << std::endl;
        } else if (ntransferred != sizeof (packet)) {
            std::cerr << "Partial data sent" << std::endl;
        }
    } else if (!ntransferred) {
        std::cout << "Client disconnecting" << std::endl;
        // Unsubscribe
        if (!m_mult.delete_polled(clientfd)) {
            std::cerr << "Failed to unsubscribe from client" << std::endl;
        }
        if (close(clientfd) == -1) {
            std::cerr << "Failed to close client socket: " << strerror(errno) << std::endl;
        }
    } else {
        // ignore failed request
        std::cerr << "Request incomplete or read failed: " << strerror(errno) << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc > 2) {
        std::cerr << "Usage: ntp-server [<port number>]" << std::endl;
        return EXIT_FAILURE;
    }
    uint16_t server_port = NTP_DEFAULT_PORT;
    if (argc == 2) {
        if (!(std::istringstream(argv[1]) >> server_port)) {
            std::cerr << "Invalid port number provided" << std::endl;
            return EXIT_FAILURE;
        }
    }

    try {
        NTPServer server(server_port);
        server.run();
    } catch (std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
