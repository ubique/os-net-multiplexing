#ifndef OS_NET_MULTIPLEXING_UTILS_H
#define OS_NET_MULTIPLEXING_UTILS_H

#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <dirent.h>
#include <errno.h>
#include <cstdint>
#include <iostream>
#include <vector>

char const *mess = "some men interpret nine memos\n";       // string to invert
char const *pali = "somem enin terpretni nem emos\n\n";     // inverted string
const int bufferLen = 1024;
const int NUMBER_OF_CLIENTS = 5;

char *proccessPalindrome(char *buffer, int res) {
    for (int i = 0; i < res / 2; ++i)
        std::swap(buffer[i], buffer[res - 2 - i]);
    return buffer;
}

void printError(const char *message) {
    std::cerr << message << '\n';
    exit(EXIT_FAILURE);
}

void printSysError(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void processActivity(int activity) {
    if (activity == 0)
        printError("Timeout.");
    if (activity < 0)
        switch (errno) {
            case EBADF:
                printError("File Descriptor Error");
                break;
            case EINVAL:
                printError("Sockets must be natural");
                break;
            case ENOMEM:
                printError("Memory cannot be allocated");
                break;
            default:
                printError("¯\\_(ツ)_/¯");
                break;
        }
}

void writeStr(char const *s) {
    ssize_t cur = 0, len = static_cast<ssize_t>(strlen(s)), tmp;
    while (cur < len) {
        if ((tmp = write(1, s + cur, static_cast<size_t>(len - cur))) == -1)
            printSysError("write");
        cur += tmp;
    }
}

void sendAll(int socket, const char *buffer) {
    size_t length = strlen(buffer);
    ssize_t m = 0, bytesSent = 0;
    while ((length - bytesSent > 0) && ((m = send(socket, buffer + bytesSent, length - bytesSent, 0)) > 0)) {
        if (static_cast<int>(m) < 0)
            printError("Error was occurred while reading");
        bytesSent += m;
    }
}

void closeSocket(int socket) {
    if (close(socket) == -1)
        printSysError("close");
}

int sockRead(int socket, char *buffer) {
    ssize_t n = 0, bytesReceived = 0;
    while ((n = recv(socket, buffer + bytesReceived, static_cast<size_t>(bufferLen - bytesReceived), 0)) > 0) {
        bytesReceived += n;
        if (buffer[bytesReceived - 1] == '\n')
            break;
    }
    return static_cast<int>(n);
}

#endif //OS_NET_MULTIPLEXING_UTILS_H
