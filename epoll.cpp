#include "epoll.h"

#include <cstring>

#include <unistd.h>

namespace
{
	template<typename T>
	T check(T arg)
	{
		if (arg == -1)
		{
			throw epoll::epoll_error(std::strerror(errno));
		}
		return arg;
	}
}

epoll::epoll() : efd_(check(epoll_create(1))) { }

epoll::~epoll()
{
	close(efd_);
}

void epoll::add(int fd, uint32_t events)
{
	auto event = epoll_event{};
	event.events = events;
	event.data.fd = fd;
	check(epoll_ctl(efd_, EPOLL_CTL_ADD, fd, &event));
}

void epoll::remove(int fd)
{
	check(epoll_ctl(efd_, EPOLL_CTL_DEL, fd, nullptr));
}

void epoll::modify(int fd, uint32_t events)
{
	auto event = epoll_event{};
	event.events = events;
	event.data.fd = fd;
	check(epoll_ctl(efd_, EPOLL_CTL_MOD, fd, &event));
}

int epoll::wait(epoll_event* buffer, size_t length, int timeout)
{
	return check(epoll_wait(efd_, buffer, length, timeout));
}
