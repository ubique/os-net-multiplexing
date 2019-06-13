#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include "client.h"
#include "protocol.h"

NTPClient::NTPClient(const std::string &hostname, uint16_t port)
{
    struct addrinfo hints = {};
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // Stream socket
    hints.ai_flags    = 0;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* server_addr;  // NTP server address
    int s = getaddrinfo(hostname.c_str(), std::to_string(port).c_str(), &hints, &server_addr);
    if (s != 0) {
        throw std::runtime_error(std::string("getaddrinfo failed: ") + gai_strerror(s));
    }

    m_socketfd = open_socket(server_addr);

    m_mult.add_polled(STDIN_FILENO, Multiplexer::POLLIN);
}

NTPClient::~NTPClient()
{
    if (close(m_socketfd) == -1) {
        std::cerr << "Failed to close the socket: " << strerror(errno) << std::endl;
    }
}

void NTPClient::run() const
{
    print_help();
    bool disconnect_requested = false;

    for (;;) {
        auto ready = m_mult.get_ready();

        for (auto event : ready) {
             if (event.fd == m_socketfd) {
                 m_mult.delete_polled(m_socketfd);
                 if (event.type == Multiplexer::POLLIN) {
                     time_t time = receive_response();
                     if (time) {
                         std::cout << "Time: " << ctime(&time);
                     }
                 } else if (event.type == Multiplexer::POLLOUT) {
                     send_request(disconnect_requested);
                     if (!disconnect_requested) {
                         m_mult.add_polled(m_socketfd, Multiplexer::POLLIN);
                     } else {
                         return;
                     }
                 }

             } else if (event.fd == STDIN_FILENO) {
                 std::string cmd;
                 std::getline(std::cin, cmd);
                 if (cmd == "ask") {
                     m_mult.add_polled(m_socketfd, Multiplexer::POLLOUT);
                 } else if (cmd == "quit") {
                     m_mult.add_polled(m_socketfd, Multiplexer::POLLOUT);
                     disconnect_requested = true;
                 } else {
                     print_help(true);
                 }
             }
        }
    }
}

void NTPClient::print_help(bool error)
{
    (error ? std::cerr : std::cout) << "Available commands: ask, quit" << std::endl;
}

int NTPClient::open_socket(addrinfo *addr) const
{
    // getaddrinfo() returns a list of address structures
    // Trying each until we successfully connect
    int sockfd = -1;
    struct addrinfo* rp;
    for (rp = addr; rp != nullptr; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd != -1) {
            if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
                break;  // Success
            }

            close(sockfd);
            sockfd = -1;
        }
    }

    freeaddrinfo(addr);

    // All addresses failed
    if (rp == nullptr) {
        std::error_code ec(errno, std::system_category());
        throw std::system_error(ec, "Failed to open and connect a socket");
    }

    return sockfd;
}

void NTPClient::send_request(bool empty) const
{
    ntp_packet packet = {};
    packet.li_vn_mode = 0b11'011'011; // li = 3 (unknown), vn = 3 (version), mode = 3 (client)

    // Send a request
    if (write(m_socketfd, &packet, empty ? 0 : sizeof(ntp_packet)) == -1) {
        close(m_socketfd);
        std::error_code ec(errno, std::system_category());
        throw std::system_error(ec, "Write failed");
    }
}

time_t NTPClient::receive_response() const
{
    ntp_packet packet = {};
    ssize_t ntransferred = read(m_socketfd, &packet, sizeof(ntp_packet));
    if (ntransferred == -1) {
        close(m_socketfd);
        std::error_code ec(errno, std::system_category());
        throw std::system_error(ec, "Read failed");
    } else if (ntransferred != sizeof (packet)) {
        std::cerr << "Partial data read" << std::endl;
        return 0;
    }

    // Using time-stamp seconds: time when the packet left the NTP server,
    // in seconds passed since 1900
    // ntohl() converts the bit/byte order from the network's to host's "endianness"

    packet.txTm_s = ntohl(packet.txTm_s); // seconds
    packet.txTm_f = ntohl(packet.txTm_f); // fractions of a second

    // Subtract 70 years from the NTP epoch time to get UNIX epoch
    // (seconds passed since 1970)
    return static_cast<time_t>(packet.txTm_s - NTP_TIMESTAMP_DELTA);
}

int main(int argc, char* argv[])
{
    std::string server_hostname = NTP_DEFAULT_SERVER;
    uint16_t server_port = NTP_DEFAULT_PORT;
    if (argc > 3) {
        std::cerr << "Usage: ntp-client [<server hostname or address> [<server port>]]" << std::endl;
        return EXIT_FAILURE;
    }

    if (argc >= 2) {
        server_hostname = argv[1];
    }

    if (argc == 3) {
        if (!(std::istringstream(argv[2]) >> server_port)) {
            std::cerr << "Invalid port number provided" << std::endl;
            return EXIT_FAILURE;
        }
    }

    try {
        NTPClient client(server_hostname, server_port);
        client.run();
    } catch (std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
