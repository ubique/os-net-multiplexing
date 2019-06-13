#ifndef DEBUG_ECHO_CLIENT_H
#define DEBUG_ECHO_CLIENT_H

#include <cstdint>
#include <iosfwd>
#include <filesystem>

struct debug_echo_client
{
	void run(const std::filesystem::path& p, const char* msg, size_t length, uint16_t requests, std::ostream& log_ostream);
};

#endif // DEBUG_ECHO_CLIENT_H