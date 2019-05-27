#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include "diffOS.c"

#define MAX_BUFF_SIZE 1024

int main (int argc, char const*argv[]) {

	int port;
	int max_connection;
	int sock;
	struct sockaddr_in server_addr;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <port> <max_connection_num>\n", argv[0]);
		exit(0);
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(atoi(argv[1]));

	if (bind(sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
		fprintf(stderr, "bind error %s\n", strerror(errno));
		close(sock);
		exit(1);
	}

	if (listen(sock, atoi(argv[2])) < 0) {
		fprintf(stderr, "listen error\n");
		close(sock);
		exit(2);
	}

	int multiplexer	= create();

	int buff[32];
	int new_sock;
	int client_sock;
	int read_len;
	int write_len;
	char *dataBuff = (char*) malloc(MAX_BUFF_SIZE);
	add(multiplexer, sock);
	while (1) {
		int numb_mpx = wait(buff, multiplexer);
		for (int i = 0; i < numb_mpx; ++i) {
			new_sock = buff[i];
			if (sock == new_sock) {
				if ((client_sock = accept(sock, (struct sockaddr*) NULL, NULL)) < 0) {
						fprintf(stderr, "accept error\n");
					}
				add(multiplexer, client_sock);
			} else {
				if ((read_len = read(new_sock, dataBuff, MAX_BUFF_SIZE)) < 0) {
					continue;
				}
				dataBuff[read_len] = '\0';
				if (read_len == 0) {
					printf("Client disconnected.\n");	
				} else {
					printf("Client tells: %s", dataBuff);
					if ((write_len = write(new_sock, dataBuff, read_len)) < 0) {
						fprintf(stderr, "write error\n");
					}
				}
			}
		}
	}
	close(sock);
	close(multiplexer);
}
