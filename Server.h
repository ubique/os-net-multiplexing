#ifndef OS_NET_MULTIPLEXING_SERVER_H
#define OS_NET_MULTIPLEXING_SERVER_H

#include "safe_poll.h"

#include <cstdint>

namespace rd {
	class passive_socket {
	public:
		int fd;

		static const int CONNECTION_BACKLOG = 100;

		bool init();

		bool bind(const char *addr, uint16_t port);

		bool listen();

		void close();
	};

	class Server {
		passive_socket sock{};

		int fds_count = 1;
		static const size_t SOCKET_BUFFER_SIZE = 1024;
		char socket_buffer[SOCKET_BUFFER_SIZE]{};

		int process_main(pollfd pfd);

		int process_fd(pollfd pfd);

		static const int CONNECTIONS_LIMIT = 100;
		pollfd fds[CONNECTIONS_LIMIT]{};


		void filter_fds();

	public:
		void start(const char *addr, uint16_t port);

		void stop();
	};
}

#endif //OS_NET_MULTIPLEXING_SERVER_H
