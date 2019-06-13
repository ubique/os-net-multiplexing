#include "debug_echo_client.h"

#include "unix_socket.h"
#include "epoll.h"

#include <vector>
#include <iterator>
#include <array>

#include <unistd.h>
#include <sys/epoll.h>

void debug_echo_client::run(const std::filesystem::path& p, const char* msg, size_t length, uint16_t requests, std::ostream& log_ostream)
{
	unix_socket socket;
	socket.set_non_blocking();
	epoll poll;
	bool connected = socket.connect(p);
	poll.add(socket.get_native_fd(), EPOLLIN | EPOLLOUT);
	std::array<epoll_event, 3> event_buf;
	std::vector<char> buffer(length);
	auto requests_left = requests;
	while (true) {
		const auto events_n = poll.wait(event_buf.data(), event_buf.size());
		for (auto i = 0; i < events_n; i++) {
			const auto events = event_buf[i].events;
			const auto cur_fd = event_buf[i].data.fd;
			if (cur_fd == socket.get_native_fd()) {
				if ((events & EPOLLERR) || (connected && (events & EPOLLHUP))) {
					log_ostream << "Server disconnected" << std::endl;
					return;
				}
				if (events & EPOLLIN) {
					const auto nrecv = socket.receive(buffer.data(), buffer.size());
					log_ostream << "Received answer: \n";
					std::copy(buffer.data(), buffer.data() + nrecv, std::ostream_iterator<char>(log_ostream));
					log_ostream << std::endl;
					if (!requests_left)
					{
						return;
					}
					poll.modify(cur_fd, EPOLLOUT);
				}
				if (events & EPOLLOUT) {
					if (connected) {
						log_ostream << "Processing request " << requests - requests_left + 1 << "..." << std::endl;
						try {
							log_ostream << "Sending..." << std::endl;
							socket.send(msg, length);
							log_ostream << "Sent: \n";
							std::copy(msg, msg + length, std::ostream_iterator<char>(log_ostream));
							log_ostream << "\nWaiting for an answer..." << std::endl;
							poll.modify(cur_fd, EPOLLIN);
						}
						catch (const unix_socket::socket_error& ex)
						{
							log_ostream << "Error: " << ex.what() << std::endl;
						}
						--requests_left;
					}
					else {
						int err = 0;
						socklen_t len = sizeof(int);
						if (getsockopt(socket.get_native_fd(), SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err != 0) {
							log_ostream << "Could not connect" << std::endl;
							return;
						}
						connected = true;
					}
				}
			}
		}
	}
}
