#include "echo_server.h"

#include "unix_server_socket.h"
#include "epoll.h"

#include <array>
#include <thread>
#include <queue>
#include <vector>
#include <ostream>
#include <map>
#include <iostream>

void echo_server::start(const std::filesystem::path& p)
{
	static constexpr size_t buffer_size = 1024 * 4;
	unix_server_socket socket;
	socket.set_non_blocking();
	socket.bind(p);
	epoll poll;
	poll.add(socket.get_native_fd(), EPOLLIN);
	std::array<epoll_event, 100> event_buf;
	std::array<char, buffer_size> buffer;
	std::map<int, unix_socket> client_sockets;
	std::map<int, std::queue<std::vector<char>>> resps;
	while (true) {
		const auto events_n = poll.wait(event_buf.data(), event_buf.size());
		for (auto i = 0; i < events_n; i++) {
			const auto events = event_buf[i].events;
			const auto cur_fd = event_buf[i].data.fd;
			if (events & (EPOLLERR | EPOLLHUP)) {
				poll.remove(cur_fd);
				client_sockets.erase(cur_fd);
				resps.erase(cur_fd);
				continue;
			}
			if (events & EPOLLIN) {
				if (cur_fd == socket.get_native_fd())
				{
					auto client = socket.accept();
					poll.add(client.get_native_fd(), EPOLLIN);
					client_sockets[client.get_native_fd()] = std::move(client);
				}
				else
				{
					const auto nrecv = client_sockets[cur_fd].receive(buffer.data(), buffer.size());
					const auto resp = std::vector<char>(buffer.begin(), buffer.begin() + nrecv);
					auto& queue = resps[cur_fd];
					queue.push(std::move(resp));
					if (queue.size() == 1) {
						poll.modify(cur_fd, EPOLLIN | EPOLLOUT);
					}
				}
			}
			if (events & EPOLLOUT) {
				auto& queue = resps[cur_fd];
				const auto& buf = queue.front();
				client_sockets[cur_fd].send(buf.data(), buf.size());
				queue.pop();
				if (queue.empty())
				{
					poll.modify(cur_fd, EPOLLIN);
				}
			}
		}
	}
}
