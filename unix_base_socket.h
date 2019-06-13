#ifndef UNIX_BASE_SOCKET
#define UNIX_BASE_SOCKET

#include <stdexcept>
#include <cstring>
#include <filesystem>

#include <sys/socket.h>
#include <sys/un.h>

struct pipe_input;
struct pipe_output;

struct unix_base_socket
{
	~unix_base_socket();

	unix_base_socket(const unix_base_socket& other) = delete;
	unix_base_socket& operator=(const unix_base_socket& other) = delete;

	unix_base_socket(unix_base_socket&& other);
	unix_base_socket& operator=(unix_base_socket&& other);

	void set_so_timeout(uint16_t millis);

	void set_non_blocking();

	int get_native_fd();

	struct socket_error : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

protected:
	unix_base_socket();
	unix_base_socket(int fd);

	sockaddr_un prepare_sockaddr(const std::filesystem::path& p);

	template<typename T>
	static T check(T arg)
	{
		if (arg == -1)
		{
			throw socket_error(std::strerror(errno));
		}
		return arg;
	}

	int sfd_;
};

#endif // UNIX_BASE_SOCKET