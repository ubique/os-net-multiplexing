//
// Created by Павел Пономарев on 2019-05-27.
//

#ifndef OS_NET_MULTIPLEXING_CLIENTEXCEPTION_H
#define OS_NET_MULTIPLEXING_CLIENTEXCEPTION_H

#include <exception>
#include <stdexcept>

class ClientException : public std::runtime_error {
using std::runtime_error::runtime_error;
};


#endif //OS_NET_MULTIPLEXING_CLIENTEXCEPTION_H
