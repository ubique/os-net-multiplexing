
#include "../tcp/TCPSocket.h"

namespace net {
    class TCPClient : public net::tcp::TCPSocket {
    public:
        TCPClient(const std::string& ipv4, const std::string& port);

    public:
    protected:
        bool afterCreate(const sockaddr_in& in) override;

    public:

        void run(const data_t& data) override;
    };

}