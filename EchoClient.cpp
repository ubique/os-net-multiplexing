#include "EchoClient.h"
#include <vector>

EchoClient::EchoClient(char *port, char *ip) {
	openSocket();
	setAddress(port, ip);
	connectSocket();
}

EchoClient::~EchoClient() {
	if (close(sock) < 0) {
		perror("Closing the socket failed");
		exit(EXIT_FAILURE);
	}
}

void EchoClient::connectSocket() {
	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("Socket connection failed");
		exit(EXIT_FAILURE);
	}
}

void EchoClient::openSocket() {
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Socket opening failed");
		exit(EXIT_FAILURE);
	}
}

void EchoClient::setAddress(char *port, char *ip) {
	addr.sin_family = AF_INET;
	addr.sin_port = (uint16_t) std::stoul(port);
	inet_pton(AF_INET, ip, &(addr.sin_addr));
}

void EchoClient::sendRequest(string request) {
	sendReq(request);
	readResponse();
}

void EchoClient::sendReq(string message) {
	uint8_t len = (uint8_t) message.length();
	const char* request = message.c_str();
	int offset = 0;

	std::vector <uint8_t> length = { len };
	if (send(sock, length.data(), 1, 0) != 1) {
		std::cerr << "Failed to send length of a message" << endl;
		exit(EXIT_FAILURE);
	}

	while (offset != message.length()) {
		int sent = send(sock, request + offset, len, 0);

		if (sent <= 0) {
			std::cerr << "Failed to send request" << endl;
			exit(EXIT_FAILURE);
		}
		
		len -= sent;
		offset += sent;
	}
}

bool EchoClient::readResponse() {
	std::vector <uint8_t> response_length(1);

	int read_length = recv(sock, response_length.data(), 1, 0);

	if (read_length == -1) {
		std::cerr << "Failed to receive length of the response" << endl;
		exit(EXIT_FAILURE);
	}
	else if (read_length == 0) {
		return false;
	}

	int length = response_length[0];

	char buffer[length];
	int received = 0;

	while (received != length) {
		int read = recv(sock, buffer + received, length - received, 0);

		if (read < 0) {
			std::cerr << "Failed to receive response" << endl;
			exit(EXIT_FAILURE);
		}
		else if (read == 0) {
			std::cerr << "Received incomplete response" << endl;
			buffer[received] = '\0';
			cout << "Server response: " << buffer << endl;
			return false;
		}
		
		received += read;
	}

	buffer[received] = '\0';
	cout << "Server response: " << buffer << endl;
	return true;
}

void EchoClient::launch() {
	struct epoll_event event;
	int epollFd = epoll_create(1);

	if (epollFd == -1) {
		perror("Failed to create epoll");
		exit(EXIT_FAILURE);
	}

	event.data.fd = sock;
	event.events = EPOLLIN;

 	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, sock, &event) == -1) {
		perror("Failed to add file descriptor to epoll");
		exit(EXIT_FAILURE);
	}

	event.data.fd = 0;
	event.events = EPOLLIN;

	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, 0, &event) == -1) {
		perror("Failed to add file descriptor to epoll");
		exit(EXIT_FAILURE);
	}

	cout << "Enter 'exit' or 'quit' to stop" << endl;
	cout << "Enter your requests:" << endl;

	struct epoll_event events[2];
	
	bool flag = true;

	while (flag) {
		int num = epoll_wait(epollFd, events, 2, -1);
		if (num == -1) {
			perror("epoll_wait error");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < num; i++) {
			if (events[i].data.fd == sock) {
				if (!readResponse()) {
					flag = false;
					continue;
				}
			}
			else if (events[i].data.fd == 0) {
				string request;
				cin >> request;
				if ((strcmp(request.c_str(), "exit") == 0) || (strcmp(request.c_str(), "quit") == 0)) {
					break;
				}
				sendRequest(request);
			}
		}
	}
}
