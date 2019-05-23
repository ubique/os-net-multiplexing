#include "Client.h"

#include <cstdio>
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <error.h>
#include <cerrno>
#include <cstdlib>
#include <iostream>

namespace rd {
	bool active_socket::init() {
		fd = socket(AF_INET, SOCK_STREAM, 0);
		return fd != -1;
	}

	bool active_socket::connect(const char *addr, uint16_t port) {
		struct sockaddr_in client = {
				.sin_family = AF_INET,
				.sin_port = htons(port),
		};
		client.sin_addr.s_addr = inet_addr(addr);
		int i = ::connect(fd, (struct sockaddr *) &client, sizeof(client));
		return i == 0;
	}

	void active_socket::close() {
		::close(fd);
	}

	std::atomic<int> Client::id_generator{0};

	void Client::run(const char *addr, uint16_t port, int input_fd, int output_fd) {
		fprintf(stderr, "Client #%d is running: %s\\%d\n", id, addr, port);

		if (!sock.init()) {
			error(EXIT_FAILURE, errno, "Creating socket failed, client #%d", id);
		}
		if (!sock.connect(addr, port)) {
			error(EXIT_FAILURE, errno, "Connecting socket failed, client #%d", id);
		}

		fprintf(stderr, "Client #%d connected\n", id);

		struct pollfd fds[2];
		auto const &socket_pfd = fds[0] = {
				.fd = sock.fd,
				.events = POLLIN
		};
		auto const &stdin_pfd = fds[1] = {
				.fd = input_fd,
				.events = POLLIN
		};

		nfds_t timeout = 1;
		while (true) {
			try_poll(fds, 2, timeout);

			if (socket_pfd.revents & POLLHUP) {
				fprintf(stderr, "Client %d closed", id);
				break;
			}

			if (socket_pfd.revents & POLLIN) {
				int len = try_read(socket_pfd.fd, socket_buffer, SOCKET_BUFFER_SIZE);
				if (len == 0) {
					continue;
				}
				write(output_fd, socket_buffer, len); //trace message
			}
			if (stdin_pfd.revents & POLLIN) {
				int len = try_read(stdin_pfd.fd, stdin_buffer, STDIN_BUFFER_SIZE);
				if (len == 0) {
					continue;
				}
				int k = write(socket_pfd.fd, stdin_buffer, len);
				if (len == k) {
					fprintf(stderr, "Client #%d passed throught %d bytes\n", id, len);
				} else {
					error(EXIT_FAILURE, errno, "Passing through failed, client #%d", id);
				}
			}
			if (stdin_pfd.revents != 0) {
				pause();
			}
		}
	}

	void Client::close() {
		sock.close();
	}

	Client::Client() : id(++id_generator) {}
}
