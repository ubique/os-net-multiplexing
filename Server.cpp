#include "Server.h"

#include <poll.h>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>
#include <cstdint>
#include <cerrno>
#include <error.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>


namespace rd {
	bool passive_socket::init() {
		fd = socket(AF_INET, SOCK_STREAM, 0);
		return fd != -1;
	}

	bool passive_socket::bind(const char *addr, uint16_t port) {
		struct sockaddr_in in = {
				.sin_family = AF_INET,
				.sin_port = htons(port),
				.sin_addr.s_addr = inet_addr(addr)
		};
		return ::bind(fd, (struct sockaddr *) &in, sizeof(sockaddr_in)) == 0;
	}

	bool passive_socket::listen() {
		return ::listen(fd, CONNECTION_BACKLOG) >= 0;
	}

	void passive_socket::close() {
		::close(fd);
	}

	int Server::process_main(pollfd pfd) {
		if (pfd.revents & POLLIN) {
			struct sockaddr_in client{};
			socklen_t client_struct_len = sizeof(client);
			int new_fd = accept(pfd.fd, (struct sockaddr *) &client, &client_struct_len);
			if (new_fd < 0) {
				return -1;
			}
			return new_fd;
		}
		return 0;
	}

	int Server::process_fd(pollfd pfd) {
		if (pfd.revents == 0) {
			//nothing new
			return 0;
		}
		/*if (fd.revents != POLLIN) {
			error(EXIT_FAILURE, errno, "Error occured, fds=%d, revents=%d", i, fd.revents);
			break;
		}*/
		if (pfd.revents & POLLIN) {
			int len = try_read(pfd.fd, socket_buffer, SOCKET_BUFFER_SIZE);
			if (len == 0) {
				return 0;
			}
			fprintf(stderr, "New message from %d, len=%d: %s\n", pfd.fd, len, socket_buffer);

			ssize_t sent = send(pfd.fd, socket_buffer, len, 0);//todo

			if (sent == -1) {
				//sending failed
				error(EXIT_FAILURE, errno, "Error during send to %d\n", pfd.fd);
			}
			if (sent == 0) {
				//socket closed on the other side
				close(pfd.fd);
				fprintf(stderr, "End of stream %d\n", pfd.fd);
				return 0;
			}
			return sent;
		} else if (pfd.revents != 0) {
			pause();
		}
		return 0;
	}

	void Server::filter_fds() {
		int last = fds_count - 1;
		for (int i = 0; i < last; ++i) {
			if (fds->fd == -1) {
				while (fds[last].fd == -1 && last > i) {
					--last;
				}
				std::swap(fds[i], fds[last]);
			}
		}
		fds_count = last + 1;
	}

	void Server::start(const char *addr, uint16_t port) {
		fprintf(stderr, "Server is starting: %s\\%d\n", addr, port);

		if (!sock.init()) {
			error(EXIT_FAILURE, errno, "Creating socket failed");
		}
		if (!sock.bind(addr, port)) {
			error(EXIT_FAILURE, errno, "Binding socket failed");
		}
		if (!sock.listen()) {
			error(EXIT_FAILURE, errno, "Listening socket failed");
		}

		fprintf(stderr, "Server is listening\n");

		memset(fds, 0, sizeof(fds));

		fds[0] = {
				.fd = sock.fd,
				.events = POLLIN
		};
		int timeout = 50;//milliseconds
		while (true) {
			try_poll(fds, fds_count, timeout);

			if (errno == EINTR) {
				pause();
			}
			if (fds[0].revents & POLLHUP) {
				fprintf(stderr, "Server stopped\n");
				break;
			}

			int new_fd = process_main(fds[0]);
			if (new_fd < 0) {
				error(EXIT_FAILURE, errno, "Accepting failed");
			}

			filter_fds();

			if (new_fd > 0) {
				if (fds_count == CONNECTIONS_LIMIT) {
					error(EXIT_FAILURE, errno, "Limit of connections exceeded %d", CONNECTIONS_LIMIT);
				}
				fds[fds_count].fd = new_fd;
				fds[fds_count].events = POLLIN;
				++fds_count;
				fprintf(stderr, "Accepted new client with fd=%d\n", new_fd);
			}
			for (int i = 1; i < fds_count; ++i) {
				process_fd(fds[i]);
			}
		}
	}

	void Server::stop() {
		sock.close();
	}
}