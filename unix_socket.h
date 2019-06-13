#ifndef UNIX_SOCKET
#define UNIX_SOCKET

#include "unix_base_socket.h"

struct unix_socket : unix_base_socket
{
	unix_socket();

	void bind(const std::filesystem::path& p);
	bool connect(const std::filesystem::path& p);

	void send(const char* buffer, size_t length);
	size_t receive(char* buffer, size_t length);

private:
	explicit unix_socket(int fd);

	friend struct unix_server_socket;
};

#endif // UNIX_SOCKET