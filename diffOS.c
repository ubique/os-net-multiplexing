#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <assert.h>
#ifdef __linux
#include <sys/epoll.h>
#endif
#ifdef __APPLE__
#include <sys/event.h>
#endif

#define EVENT_BUFF_SIZE 32

#ifdef __linux
	struct epoll_event epoll_struct, event_list[EVENT_BUFF_SIZE];
#elif __APPLE__
	struct kevent kevent_struct, event_list[EVENT_BUFF_SIZE];
#endif

int create() {
	int multiplexer = -1;
	#ifdef __linux
	multiplexer = epoll_create1(0);
	if (multiplexer < 0) {
		fprintf(stderr, "create epoll error\n");
		exit(1);
	}
	#elif __APPLE__ 
	multiplexer = kqueue();
	if (multiplexer < 0) {
		fprintf(stderr, "create kqueue error\n");
		exit(1);
	}
	#endif
	return multiplexer;
}

int wait(int *buff, int multiplexer, int *flags) {
	int numb = 0;
	#ifdef __linux
	numb = epoll_wait(multiplexer, event_list, EVENT_BUFF_SIZE, -1);
	#elif __APPLE__ 
	numb = kevent(multiplexer, NULL, 0, EVENT_BUFF_SIZE, NULL);
	#endif
	for (int i = 0; i < numb; ++i) {
		buff[i] = event_list[i].data.fd;
		flags[i] = event_list[i].events & EPOLLOUT;
	}
	return numb;
}

int add(int multiplexer, int sock, int shouldOutput) {
	#ifdef __linux
	epoll_struct.data.fd = sock;
	if (shouldOutput) {
		epoll_struct.events = EPOLLOUT | EPOLLET | EPOLLIN;
	} else {
		epoll_struct.events = EPOLLIN | EPOLLET;
	}
	if (epoll_ctl(multiplexer, EPOLL_CTL_ADD, sock, &epoll_struct) < 0) {
		fprintf(stderr, "epoll error %s\n", strerror(errno));
		exit(1);
	}
	#elif __APPLE__
	EV_SET(&kevent_struct, sock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	if (kevent(multiplexer, &kevent_struct, 1, NULL, 0, NULL) < 0) {
		fprintf(stderr, "kevent error\n");
		exit(1);
	}
	#endif
	return 0;
}

void del(int multiplexer, int sock) {
	#ifdef __linux
	if (epoll_ctl(multiplexer, EPOLL_CTL_DEL, sock, NULL) < 0) {
		fprintf(stderr, "Cannot delete multiplexer\n");
		exit(5);
	}
	#elif __APPLE__
	EV_SET(&kevent_struct, sock, 0, EV_DELETE, 0, 0, NULL);
	if (kevent(multiplexer, &kevent_struct, 1, NULL, 0, NULL) < 0) {
		fprintf(stderr, "Cannot delete multiplexer\n");
	}
	#endif
}