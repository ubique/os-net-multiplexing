#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>

#include <iostream>

using namespace std;
#define MAX_EVENTS 10

int main(int argc, char * argv[]) {

	if (argc == 1) {
		cout << "port number exprected"<< endl;
		return 1;
	}

	int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sock < 0) {
		perror("error with sockets");
		return 1;
	}

	int port = atoi(argv[1]);

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("error with connection");
		return 2;
	}

	char info[] = "test";
	int info_size = strlen(info);
	char * info_pointer = info;

	// new code (sending)
	int epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create");
		close(sock);
		exit(EXIT_FAILURE);
	}

	cout << "after create" << endl;

	struct epoll_event ev;
	struct epoll_event events[MAX_EVENTS];
	ev.events  = EPOLLIN | EPOLLOUT;
	ev.data.fd = sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
		perror("epoll_ctl: sock");
		close(sock);
		close(epollfd);
		exit(EXIT_FAILURE);
	}
	ev.events = EPOLLIN;

	cout << "after ctl" << endl;

	int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
	cout << "after nfds, nfds = " << nfds << endl;
	if (nfds == -1) {
		perror("epoll_wait");
		close(sock);
		close(epollfd);
		exit(EXIT_FAILURE);
	}



	for (int i = 0; i < nfds; i++) {
		if (events[i].data.fd == sock) {
			while (info_size > 0) {
				int sended = send(sock, info_pointer, info_size, 0);
				if (sended == -1) {
					perror("send");
					exit(EXIT_FAILURE);
				}
				info_pointer += sended;
				info_size -= sended;
			}
		}
	}

	shutdown(sock, SHUT_WR);

	if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sock, &ev) == -1) {
		perror("epoll_ctl: sock");
		close(sock);
		close(epollfd);
		exit(EXIT_FAILURE);
	}

	nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
	if (nfds == -1) {
		perror("epoll_wait");
		close(sock);
		close(epollfd);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < nfds; i++) {
		if (events[i].data.fd == sock) {
			int sockk = events[i].data.fd;
			if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sockk, NULL) == -1) {
				perror("epoll_ctl: delete sock");
				close(sockk);
				continue;
			}

			char buf[1024];
			char * rec_pointer = buf;
			int rec_info = 0, rec_size = 0;

			while((rec_info = recv(sockk, rec_pointer, 1024, 0)) > 0) {
				rec_pointer += rec_info;
				cout << buf << endl;
			}

			if (rec_info == -1) {
				perror(strerror(errno));
			}

			if (rec_info == 0) {
				perror(strerror(errno));
			}
			close(sock);
		}
	}

	close(sock);

	return 0;
}
