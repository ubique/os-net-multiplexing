#include "Server.h"
#include "Client.h"

#include <cstdlib>
#include <cerrno>
#include <cstdint>
#include <error.h>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <cassert>
#include <cstring>
#include <cstring>
#include <thread>
#include <vector>

using namespace std::literals::chrono_literals;
using namespace std::string_literals;

auto input_file = "input.txt";

char buf[BUFSIZ] = {};

//inline int len_of(int i) { return strlen(data[i]); }


template<int N>
void init(const char *addr, uint16_t port, std::array<std::vector<std::string>, N> data) {
	rd::Server server{};
	std::thread thread_server([&] {
		server.start(addr, port);
	});

	std::this_thread::sleep_for(200ms);

	std::array<rd::Client, N> clients{};
	std::array<std::thread, N> thread_clients{};
	std::array<int[2], N> pipes_to_read{};
	std::array<int[2], N> pipes_to_write{};

	for (int i = 0; i < N; ++i) {
		assert(pipe(pipes_to_read[i]) == 0);
		assert(pipe(pipes_to_write[i]) == 0);

		thread_clients[i] = std::thread([&, i] {
			clients[i].run(addr, port, pipes_to_read[i][0], pipes_to_write[i][1]);
		});
	}

	std::this_thread::sleep_for(500ms);

	for (int i = 0; i < N; ++i) {
		for (const auto &item : data[i]) {
			int length = item.length();
			int k = write(pipes_to_read[i][1], item.c_str(), length);
			if (k != length) {
				fprintf(stderr, "\nWrite #%d : expected=%d, actual=%d\n", i, length, k);
			} else {
				fprintf(stderr, "\nWrote to pipe\n");
			}
			std::this_thread::sleep_for(100ms);
		}
	}

	std::this_thread::sleep_for(500ms);

	for (int i = 0; i < N; ++i) {
		for (const auto &item : data[i]) {
			int length = item.length();
			int k = read(pipes_to_write[i][0], buf, length);
			if (k != length) {
				fprintf(stderr, "\nRead #%d : expected=%d, actual=%d\n", i, length, k);
			} else {
				if (memcmp(item.c_str(), buf, k) == 0) {
					fprintf(stderr, "\nAccepted correct %d bytes\n", k);
				} else {
					fprintf(stderr, "\nAccepted wrong %d bytes\n", k);
				}
			}
		}
	}

	std::this_thread::sleep_for(500ms);

	server.stop();
	for (int i = 0; i < N; ++i) {
		clients[i].close();
	}
	for (int i = 0; i < N; ++i) {
		thread_clients[i].join();
	}
	thread_server.join();
}


void write_to_stdin(const char *string) {
//	pid_t pid = getpid();
//	int status = system(std::string("echo "s + string + " > " + "/proc/" + std::to_string(pid) + "/fd/0").c_str());
}

int main(int argc, char **argv) {
	--argc;
	if (argc != 2) {
		error(EXIT_FAILURE, errno, "Wrong number of args:%d\n", argc);
		return -1;
	}
	const char *addr = argv[1];
	uint16_t port = strtol(argv[2], nullptr, 10);

	init<4>(addr, port, {
			std::vector<std::string>{"a"s, "b"s},
			std::vector<std::string>{"c"s, "d"s},
			std::vector<std::string>{"e"s, "f"s},
			std::vector<std::string>{"abc"}
	});

	return 0;
}