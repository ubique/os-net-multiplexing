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

	sock = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));
	addr.sin_addr.s_addr = inet_addr(argv[1]);

	if (connect(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		fprintf(stderr, "connect error %s\n", strerror(errno));
		exit(1);
	}

	int multiplexer	= create();
	int buff[32];
	int new_sock;
	int client_sock;
	int read_len;
	int write_len;
	char *dataBuff = (char*) malloc(MAX_BUFF_SIZE);
	char message[MAX_BUFF_SIZE];
	add(multiplexer, sock);
	add(multiplexer, STDIN_FILENO);

	while(1) {
		int numb_mpx = wait(buff, multiplexer);
		for (int i = 0; i < numb_mpx; ++i) {
			new_sock = buff[i];
			if (new_sock == STDIN_FILENO) {
				fgets(message, MAX_BUFF_SIZE, stdin);
				if (strcmp(message, "exit\n") == 0) {
					close(multiplexer);
					close(new_sock);
					close(sock);
					return 0;
				}
				int wr;
				if ((wr = write(sock, message, strlen(message))) < 0) {
					fprintf(stderr, "write error\n");
					close(sock);
				}
			} else {
				if((read_len = recv(new_sock, dataBuff, MAX_BUFF_SIZE, 0)) < 0) {
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
				printf("Server tells: %s", dataBuff);
			}
		}
	}
}