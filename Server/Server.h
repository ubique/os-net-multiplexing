//
// Created by Noname Untitled on 25.05.19.
//

#pragma once

#define BUFFER_SIZE 1024
#define MIN_PORT 2'001
#define MAX_PORT 64'000

class Server {
public:
    explicit Server(char *);

    ~Server();

    void createBinding();

    void run();

private:
    int mPort;
    int mServerSocket;
    struct sockaddr_in mServerAddress;

    std::set<int> mConnectedSockets;
};