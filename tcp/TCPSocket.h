#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "../utils/Utils.h"
#include <sys/epoll.h>
#include <sstream>

#define STDINFD 0

namespace net {
    namespace tcp {

        class TCPSocket {
        public:
            TCPSocket(std::string ipv4, const std::string& port);

            /**
             * Function create socket and call {@link TCPSocket::afterCreate} for ip:port
             * @return true if was created successfully, false otherwise
             */
            virtual bool create();

            virtual void run(const data_t& data = data_t()) = 0;

        protected:

            int getFileDescriptor();

            virtual bool afterCreate(const sockaddr_in& in) = 0;


            std::string fullAddress() {
                return ipv4 + ":" + std::to_string(port);
            }


            void epollCtl(int events, int descriptor);


            int epoll;


            int getDescNumber(bool exitIfFail) {
                int res = epoll_wait(epoll, changing, MAX_EVENTS, -1);
                if (res == -1) {
                    perror("epoll_wait");
                    if (exitIfFail) {
                        exit(EXIT_FAILURE);
                    }
                }
                return res;
            }

            const static int MAX_EVENTS = 4;

            struct epoll_event event{};
            struct epoll_event changing[MAX_EVENTS];


            void epollAttach();


            int read(const int& descriptor, data_t& receivedData, const int& targetSize = -1) const {
                size_t bufferSize = 128;
                data_t buffer(bufferSize);
                size_t bytesGet;
                long count = 0;
                while (targetSize == -1 || receivedData.size() < targetSize) {
                    bytesGet = recv(descriptor, buffer.data(), bufferSize, 0);
                    if (bytesGet == -1) {
                        Utils::fail("Error while receiving (ECHOProtocol::receiver function)");
                        return -1;
                    } else {
                        count += bytesGet;
                        for (auto i = 0; i < bytesGet; i++) {
                            receivedData.push_back(buffer[i]);
                        }
                    }
                    if (bytesGet == 0 || bytesGet < bufferSize) {
                        if (targetSize != -1 && receivedData.size() != targetSize) {
                            Utils::fail("Not all excepted data received");
                            return -1;
                        }
                        break;
                    }
                }
                std::stringstream received;
                received << "Received " << count << " bytes: \n";
                for (auto c: receivedData) {
                    received << c;
                }
                Utils::message(received.str());
                return count;
            }

        private:


            /**
             * Connection file descriptor
             */

            int fileDescriptor;

            /**
             * Connection address
             */
            std::string ipv4;

            /**
             * Connection port
             */
            int port;


        };

    }
}