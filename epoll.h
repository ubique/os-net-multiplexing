#ifndef EPOLL
#define EPOLL

#include <stdexcept>

#include <sys/epoll.h>

struct epoll
{
	epoll();
	~epoll();

	struct epoll_error : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	void add(int fd, uint32_t events);
	void remove(int fd);
	void modify(int fd, uint32_t events);

	int wait(epoll_event* buffer, size_t length, int timeout = -1);

private:
	int efd_;
};

#endif // EPOLL
