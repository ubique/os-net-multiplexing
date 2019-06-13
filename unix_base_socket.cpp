#include "unix_base_socket.h"

#include <cstring>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>

unix_base_socket::unix_base_socket() : sfd_(socket(AF_UNIX, SOCK_STREAM, 0))
{
	check(sfd_);
}

unix_base_socket::unix_base_socket(int fd) : sfd_(fd) { }

unix_base_socket::~unix_base_socket()
{
	close(sfd_);
}

unix_base_socket::unix_base_socket(unix_base_socket&& other) : sfd_(other.sfd_)
{
	other.sfd_ = -1;
}

unix_base_socket& unix_base_socket::operator=(unix_base_socket&& other)
{
	close(sfd_);
	sfd_ = other.sfd_;
	other.sfd_ = -1;
	return *this;
}

void unix_base_socket::set_so_timeout(uint16_t millis)
{
	auto timeout = timeval{ 0, millis * 100 };
	check(setsockopt(sfd_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout)));
	check(setsockopt(sfd_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout)));
}

void unix_base_socket::set_non_blocking()
{
	check(fcntl(sfd_, F_SETFL, check(fcntl(sfd_, F_GETFL)) | O_NONBLOCK));
}

int unix_base_socket::get_native_fd()
{
	return sfd_;
}

sockaddr_un unix_base_socket::prepare_sockaddr(const std::filesystem::path& p)
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
	return addr;
}
