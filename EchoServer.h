#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <string>
#include <iostream>

using namespace std;

class EchoServer {
	public:
		EchoServer(char *, char *);
		~EchoServer();

		void openListener();
		void setAddress(char *, char *);
		void launch();
	private:
		int listener;
		int epollFd;
		struct sockaddr_in addr;
		bool sendResponse(int, string);
		bool readRequest(int, int, string&);
	
};
