#ifndef OS_NET_MULTIPLEX_IHANDLE_H
#define OS_NET_MULTIPLEX_IHANDLE_H


class IHandle {
public:
    virtual ~IHandle() = default;
    virtual void handle(int ops) = 0;
};


#endif //OS_NET_MULTIPLEX_IHANDLE_H
