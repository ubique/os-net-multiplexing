#ifndef OS_NET_MULTIPLEXING_CLIENT_H
#define OS_NET_MULTIPLEXING_CLIENT_H

#include "safe_poll.h"

#include <cstdint>
#include <atomic>

namespace rd {
	struct active_socket {
		int fd;

		bool init();

		bool connect(const char *addr, uint16_t port);

		void close();
	};

	class Client {
		static std::atomic<int> id_generator;

		int id = ++id_generator;

		active_socket sock{};

		static const size_t SOCKET_BUFFER_SIZE = 1024;
		char socket_buffer[SOCKET_BUFFER_SIZE]{};

		static const size_t STDIN_BUFFER_SIZE = 1024;
		char stdin_buffer[STDIN_BUFFER_SIZE]{};

	public:
		Client();

		void run(const char *addr, uint16_t port, int input_fd = 0, int output_fd = 1);

		void close();
	};
}

#endif //OS_NET_MULTIPLEXING_CLIENT_H
