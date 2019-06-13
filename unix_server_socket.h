#ifndef UNIX_SERVER_SOCKET_H
#define UNIX_SERVER_SOCKET_H

#include "unix_socket.h"

#include <sys/socket.h>

struct unix_server_socket : unix_base_socket
{
	void bind(const std::filesystem::path& p, int backlog = SOMAXCONN);

	unix_socket accept();
};

#endif // UNIX_SERVER_SOCKET_H