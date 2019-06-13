#include "unix_socket.h"

#include <cstring>

unix_socket::unix_socket() = default;

void unix_socket::bind(const std::filesystem::path& p)
{
	const auto addr = prepare_sockaddr(p);
	check(::bind(sfd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(decltype(addr))));
}

bool unix_socket::connect(const std::filesystem::path& p)
{
	const auto addr = prepare_sockaddr(p);
	const auto con= ::connect(sfd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(decltype(addr)));
	if (con == -1 && errno == EINPROGRESS)
	{
		return false;
	}
	check(con);
	return true;
}

void unix_socket::send(const char* buffer, size_t length)
{
	while (length > 0)
	{
		const auto nsent = check(::send(sfd_, buffer, length, 0));
		length -= nsent;
		buffer += nsent;
	}
}

size_t unix_socket::receive(char* buffer, size_t length)
{
	return check(recv(sfd_, buffer, length, 0));
}

unix_socket::unix_socket(int fd) : unix_base_socket(fd) { }
