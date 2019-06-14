#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "diffOS.c"

#define MAX_BUFF_SIZE 1024


int main (int argc, char const *argv[]) {
	int port;
	int sock;
	struct sockaddr_in addr;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <ip_address> <port>\n", argv[0]);
		exit(0);
	}

	sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));
	addr.sin_addr.s_addr = inet_addr(argv[1]);

	if (connect(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		if (errno != EINPROGRESS){
			fprintf(stderr, "connect error %s\n", strerror(errno));
			exit(1);
		}
	}

	int multiplexer	= create();
	int buff[32];
	int flags[32];
	int new_sock;
	int client_sock;
	int read_len;
	int write_len;
	int writing = 0, returning = 0;
	int message_len = 0;
	int inProgress = 0;
	int connected = 0;
	char dataBuff[MAX_BUFF_SIZE];
	char message[MAX_BUFF_SIZE];
	add(multiplexer, sock, 1);
	add(multiplexer, STDIN_FILENO, 0);

	while(1) {
		int numb_mpx = wait(buff, multiplexer, flags);
		for (int i = 0; i < numb_mpx; ++i) {
			new_sock = buff[i];
			if (flags[i] == 0) {
				if (new_sock == STDIN_FILENO) {
					if (inProgress == 0) {
						fgets(message, MAX_BUFF_SIZE, stdin);
						message_len = strlen(message);
						if (strcmp(message, "exit\n") == 0) {
							close(multiplexer);
							close(new_sock);
							close(sock);
							return 0;
						}
						int wr;
						writing = 0; returning = 0;
						inProgress = 1;
						del(multiplexer, sock);
						add(multiplexer, sock, 1);
					} else {
						char tmpbuff[MAX_BUFF_SIZE];
						fgets(tmpbuff, MAX_BUFF_SIZE, stdin);
						printf("Client zanyat\n");
					}
				} 
			} else {
				if (!connected) {
					int error = 0;
				    int errlen = 0;
				    if (getsockopt(sock, SOL_SOCKET, SO_ERROR,(void*) &error, &errlen) != 0) {
				    	printf("%s\n", strerror(errno));
				        exit(2);
				    }
				    if (error == 0) {
				    	connected = 1;
				    }
				    continue;
				}
				while(writing < message_len) {
					int wr;
					if ((wr = write(sock, message + writing, message_len - writing)) < 0) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							break;
						}
						fprintf(stderr, "write error: %s\n", strerror(errno));
						close(sock);
						exit(1);
					}
					writing += wr;
				}

				while (1) {
					if((read_len = recv(new_sock, dataBuff, MAX_BUFF_SIZE, 0)) < 0) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							break;
						}
						printf("Server is closed %s\n", strerror(errno));
						close(sock);
						close(new_sock);
						close(multiplexer);
						return 0;
					}
					if (read_len == 0) {
						printf("Server disconnected\n");
						close(sock);
						close(new_sock);
						close(multiplexer);
						return 0;
					}
					dataBuff[read_len] = '\0';
					printf("Server tells: %s\n", dataBuff);
					returning += read_len;
				}
				if (returning == message_len) {
					inProgress = 0;
				}
			}
		}
	}
}