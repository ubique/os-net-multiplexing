#ifndef ECHO_SERVER_H
#define ECHO_SERVER_H

#include <filesystem>

struct echo_server
{
	void start(const std::filesystem::path& p);
};

#endif // ECHO_SERVER_H