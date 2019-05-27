//
// Created by Anton Shelepov on 2019-05-17.
//

#ifndef OS_NET_SOCKET_DESCRIPTOR_H
#define OS_NET_SOCKET_DESCRIPTOR_H

#include <unistd.h>

struct socket_descriptor {
    socket_descriptor();
    socket_descriptor(int descriptor);
    socket_descriptor(socket_descriptor&& other) noexcept;
    socket_descriptor& operator=(socket_descriptor&& other) noexcept;
    bool valid() const;
    int operator*() const;
    ~socket_descriptor();

    socket_descriptor accept();

private:
    int descriptor = -1;
};


#endif //OS_NET_SOCKET_DESCRIPTOR_H