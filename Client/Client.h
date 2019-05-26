//
// Created by Noname Untitled on 26.05.19.
//

#pragma once

#define BUFFER_SIZE 1024
#define MIN_PORT 2'001
#define MAX_PORT 64'000

class Client {
public:
    explicit Client(char *name, char *address, char *port);

    ~Client();

    void createConnection();

    void run();

private:
    char *mName;
    char *mAddress;
    int mPort;

    int mClientSocket;
};