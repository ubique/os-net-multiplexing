#include "unix_server_socket.h"

#include <cstring>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>

void unix_server_socket::bind(const std::filesystem::path& p, int backlog)
{
	auto addr = sockaddr_un{ AF_UNIX };
	const auto data = p.native().data();
	const auto size = p.native().size();
	static_assert(std::is_same<std::remove_cv_t<std::remove_pointer_t<decltype(data)>>, std::remove_extent_t<decltype(addr.sun_path)>>::value);
	if (size >= std::extent<decltype(addr.sun_path)>::value)
	{
		throw std::invalid_argument("path too long");
	}
	std::memcpy(addr.sun_path, data, size);
	addr.sun_path[size] = '\0';
	unlink(addr.sun_path);
	check(::bind(sfd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(decltype(addr))));
	check(listen(sfd_, backlog));
}

unix_socket unix_server_socket::accept()
{
	return unix_socket(check(::accept(sfd_, nullptr, nullptr)));
}