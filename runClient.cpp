#include "Client.h"

#include <cstdlib>
#include <cerrno>
#include <error.h>


int main(int argc, char **argv) {
	--argc;
	if (argc != 2) {
		error(EXIT_FAILURE, errno, "Wrong number of args:%d\n", argc);
		return -1;
	}
	const char *addr = argv[1];
	uint16_t port = strtol(argv[2], nullptr, 10);

	rd::Client client{};
	client.run(addr, port, 0, 0);

	return 0;
}