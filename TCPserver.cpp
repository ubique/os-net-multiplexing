//
// Created by domonion on 02.05.19.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <errno.h>
#include <zconf.h>
#include <cstring>
#include <string>
#include <vector>
#include <utils.hpp>
#ifdef __FressBSD__
#include <sys/event.h>
#else
#include <sys/epoll.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <set>

using std::set;
using std::cout;
using std::endl;
using std::string;
using std::vector;

int set_nonblock(int fd) {
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

const int MAX_EVENTS = 10;

int main(int argc, char **argv) {
    if (argc != 2) {
        cout << "Usage: ./server address\nExample: ./server 127.0.0.1";
        return 0;
    }
    int master = socket(AF_INET, SOCK_STREAM, 0);
    check_error(master, "socket");
    struct sockaddr_in server{}, client{};
    socklen_t size = sizeof(sockaddr_in);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    check_error(inet_pton(AF_INET, argv[1], &server.sin_addr), "inet_pton");
    check_error(bind(master, (sockaddr *) (&server), size), "bind");
    check_error(listen(master, SOMAXCONN), "listen");
    #ifdef __FreeBSD__
int kq= kqueue();
check_error(kq, "kqueue");
struct kevent ev;
memset(&ev, 0, sizeof(kevent));
EV_SET(&ev, master, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
check_error(kq, &event, 1, NULL, 0, NULL, "EV_SET");
    #else
	    int EPoll = epoll_create1(0);
	    check_error(EPoll, "epoll_create1");
	    struct epoll_event Event;
	    Event.data.fd = master;
		    Event.events = EPOLLIN;
		    check_error(epoll_ctl(EPoll, EPOLL_CTL_ADD, master, &Event), "epoll_ctl");
    #endif
    while (true) {
	int N;
	#ifdef __FreeBSD__
		struct kevent kfs[10];
		memset(kfs, 0, sizeof(kevent) * 10);
		N = kevent(kfs, NULL, 0, &kfs, 10, NULL);
	#else
        	struct epoll_event Events[MAX_EVENTS];
        	N = epoll_wait(EPoll, Events, MAX_EVENTS, -1);
	#endif
	check_error(N, "wait in multiplexing");
        for (int i = 0; i < N; i++) {
#ifdef __FreeBSD__
		if(kfs[i].ident == master)
#else
            if (Events[i].data.fd == master) 
#endif
	    {
                int slave = accept(master, (sockaddr *) (&client), &size);
                check_error(slave, "accept");
#ifdef __FreeBSD__
		struct kevent kvnt;
		memset(&kvnt, 0, sizeof(kvnt));
#else
                struct epoll_event slaveEvent;
                slaveEvent.data.fd = slave;
                slaveEvent.events = EPOLLIN;
                check_error(epoll_ctl(EPoll, EPOLL_CTL_ADD, slave, &slaveEvent), "epoll_ctl");
#endif
            } else {
		int slave;
		#ifdef __FreeBSD__
			slave = kfs.ident;
		#else	
                	slave = Events[i].data.fd;
		#endif
                char size;
                doRecv(&size, 1, slave);
                vector<char> data(size);
                doRecv(&data[0], size, slave);
                cout << &data[0] << endl;
                for (char &i : data) {
                    i += abs('A' - 'a');
                }
                data.back() = '\0';
                doSend(&data[0], size, slave);
                check_error(shutdown(slave, SHUT_RDWR), "shutdown");
                check_error(close(slave), "close");
            }
        }
    }
}
