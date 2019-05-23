#include "Server.h"

#include <cstdlib>
#include <cerrno>
#include <error.h>

int main(int argc, char **argv) {
	--argc;
	if (argc != 2) {
		error(EXIT_FAILURE, errno, "Wrong number of args: %d\n", argc - 1);
	}

	rd::Server server{};

	const char *addr = argv[1];
	uint16_t port = strtol(argv[2], nullptr, 10);

	server.start(addr, port);
}

