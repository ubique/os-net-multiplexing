#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <iostream>

using std::cout;
using std::cin;
using std::endl;
using std::string;

class EchoClient {
	public:
		EchoClient(char *, char *);
		~EchoClient();

		void connectSocket();
		void openSocket();
		void setAddress(char *, char *);
		void sendRequest(string);
		void launch();
	private:
		int sock;
		struct sockaddr_in addr;
		void sendReq(string);
		bool readResponse();
};
