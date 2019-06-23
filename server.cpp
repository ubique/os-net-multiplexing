#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

#include <iostream>

using namespace std;
#define MAX_EVENTS 10
int main(int argc, char * argv[]) {

	if (argc == 1) {
		cout << "port number exprected" << endl;
		return 1;
	}

	int listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listener < 0) {
		perror("error with socket");
		return 1;
	}

	int port = atoi(argv[1]);

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("error with bind");
		return 2;
	}

	listen(listener, 1);

	int epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create");
		close(listener);
		exit(EXIT_FAILURE);
	}

	struct epoll_event ev;
	struct epoll_event events[MAX_EVENTS];
	ev.events  = EPOLLIN;
	ev.data.fd = listener;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listener, &ev) == -1) {
		perror("epoll_ctl: listener");
		close(listener);
		close(epollfd);
		exit(EXIT_FAILURE);
	}

	for(;;) {

		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		cout << "nfds = " << nfds << endl;
		if (nfds == -1) {
			perror("epoll_wait");
			close(listener);
			close(epollfd);
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < nfds; i++) {
			if (events[i].data.fd == listener) {

				struct sockaddr_storage addr;
				socklen_t addrlen = sizeof(addr);

				int accepted = accept(listener, (struct sockaddr *) &addr, &addrlen);
				if (accepted == -1) {
					perror("accept");
					exit(EXIT_FAILURE);
				}

				char buf[1024];

				char * rec_pointer = buf;
				int rec_info = 0, rec_size = 0;

				while((rec_info = recv(accepted, rec_pointer, 1024, 0)) > 0) {
					rec_pointer += rec_info;
					cout << buf << endl;
				}

				int info_size = strlen(buf);
				char * info_pointer = buf;

				while (info_size > 0) {
					int sended = send(accepted, info_pointer, info_size, 0);
					if (sended == -1) {
						perror("send");
						exit(EXIT_FAILURE);
					}
					info_pointer += sended;
					info_size -= sended;
				}
				close(accepted);
			}
		}
	}

	return 0;
}
