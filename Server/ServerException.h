//
// Created by Павел Пономарев on 2019-05-27.
//

#ifndef OS_NET_MULTIPLEXING_SERVEREXCEPTION_H
#define OS_NET_MULTIPLEXING_SERVEREXCEPTION_H

#include <exception>
#include <stdexcept>

class ServerException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};


#endif //OS_NET_MULTIPLEXING_SERVEREXCEPTION_H
