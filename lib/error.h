//
// Created by vitalya on 27.05.19.
//

#ifndef OS_NET_ERROR_H
#define OS_NET_ERROR_H

#include <stdexcept>
#include <cstring>

void error(const std::string &msg) {
    throw std::runtime_error(msg + ": " + strerror(errno));
}

#endif //OS_NET_ERROR_H
