#include "EchoServer.h"
#include <vector>

EchoServer::EchoServer(char *port, char *ip) {
	openListener();
	setAddress(port, ip);
	if (bind(listener, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("Socket binding failed");
		exit(EXIT_FAILURE);
	}
	listen(listener, 1);
}

EchoServer::~EchoServer() {
	if (close(listener) < 0) {
		perror("Closing the socket failed");
		exit(EXIT_FAILURE);
	}
}

void EchoServer::openListener() {
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		perror("Listener socket opening failed");
		exit(EXIT_FAILURE);
	}
}

void EchoServer::setAddress(char *port, char *ip) {
	addr.sin_family = AF_INET;
	addr.sin_port = (uint16_t) std::stoul(port);
	inet_pton(AF_INET, ip, &(addr.sin_addr));
}

bool EchoServer::readRequest(int socket, int length, string& request) {
	char buffer[length];
	int received = 0;

	while (received != length) {
		int read = recv(socket, buffer + received, length - received, 0);

		if (read <= 0) {
			std::cerr << "Failed to receive request" << endl;
			return false;
		}

		received += read;
	}

	buffer[received] = '\0';
	request = buffer;
	return true;
}

bool EchoServer::sendResponse(int socket, string message) {
	const char* response = message.c_str();
	int offset = 0;
	uint8_t len = (uint8_t) message.length();

	std::vector <uint8_t> length = { len };

	if (send(socket, length.data(), 1, 0) != 1) {
		std::cerr << "Failed to send length of a message" << endl;
		return false;
	}

	while (offset != message.length()) {
		int sent = send(socket, response + offset, len, 0);

		if (sent <= 0) {
			std::cerr << "Failed to send response" << endl;
			return false;
		}

		len -= sent;
		offset += sent;
	}

	return true;
}

void EchoServer::launch() {
	struct epoll_event events[5];
	struct epoll_event currentEvent;

	int epollFd = epoll_create(1);

	if (epollFd == -1) {
		perror("Failed to create epoll");
		exit(EXIT_FAILURE);
	}
	
	currentEvent.events = EPOLLIN;
	currentEvent.data.fd = listener;

	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, listener, &currentEvent) == -1) {
		perror("Failed to add file descriptor to epoll");
		exit(EXIT_FAILURE);
	}

	while (1) {
		int num = epoll_wait(epollFd, events, 5, -1);
		if (num == -1) {
			perror("epoll_wait error");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < num; i++) {
			if (events[i].data.fd == listener) { 
				int socket = accept(listener, NULL, NULL);
				if (socket < 0) {
					perror("accept() error");
					continue;
				}

				if (fcntl(listener, F_SETFL, fcntl(listener, F_GETFD, 0) | O_NONBLOCK) < 0) {
					continue;
				}

				struct epoll_event newEvent;
				newEvent.events = EPOLLIN;
				newEvent.data.fd = socket;

				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socket, &newEvent) == -1) {
					perror("Failed to add file descriptor to epoll");
					continue;
				}

				cout << "New client has connected" << endl;
			} else {
				std::vector <uint8_t> request_length(1);
				int socket = events[i].data.fd;

				int read_length = recv(socket, request_length.data(), 1, 0);


				if (read_length == -1) {
					std::cerr << "Failed to receive length of the request" << endl;
					continue;
				}
				else if (read_length == 0) {
					std::cerr << "Client has disconnected" << endl;

					if (epoll_ctl(epollFd, EPOLL_CTL_DEL, socket, NULL) == -1) {
						perror("Failed to remove file descriptor from epoll");
					}

					if (close(socket) < 0) {
						std::cerr << "Failed to close socket" << endl;
					}
				}
				else {
					int length = request_length[0];

					string request;
					if (!readRequest(socket, length, request)) {
						continue;
					}

					cout << "Client's request: " << request << endl;
			
					if (!sendResponse(socket, request)) {
						continue;
					}
				}
			}
		}
	}
}
