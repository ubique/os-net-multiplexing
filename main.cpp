#include "echo_server.h"
#include "debug_echo_client.h"

#include <iostream>
#include <filesystem>

#include <boost/program_options.hpp>

int main(int argc, char* argv[])
{
	namespace po = boost::program_options;
	auto desc = po::options_description("General options");
	std::string mode;
	desc.add_options()
		("help", "help message")
		("mode", po::value<std::string>(&mode), "mode: 'server' or 'client'")
		;
	std::filesystem::path addr;
	auto server_desc = po::options_description("Server options");
	server_desc.add_options()
		("path", po::value<std::filesystem::path>(&addr)->default_value("/tmp/os-descriptor-passing"), "path to bind to")
		;
	auto client_desc = po::options_description("Client options");
	std::string message;
	uint16_t requests;
	client_desc.add_options()
		("path", po::value<std::filesystem::path>(&addr)->default_value("/tmp/os-descriptor-passing"), "path to bind to")
		("msg", po::value<std::string>(&message)->default_value("Hello World!"), "message to send")
		("n", po::value<uint16_t>(&requests)->default_value(10), "number of requests to send")
		;
	const auto p = po::positional_options_description().add("mode", 1);
	po::variables_map vm;
	try
	{
		store(po::command_line_parser(argc, argv).options(desc).positional(p).allow_unregistered().run(), vm);
		notify(vm);
	}
	catch (const po::error& ex)
	{
		std::cout << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	if (vm.count("help"))
	{
		desc.add(server_desc).add(client_desc);
		std::cout << desc << std::endl;
		return EXIT_SUCCESS;
	}
	if (vm.count("mode"))
	{
		if (mode == "server")
		{
			desc.add(server_desc);
			try
			{
				store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
				notify(vm);
			}
			catch (const po::error& ex)
			{
				std::cout << ex.what() << std::endl;
				return EXIT_FAILURE;
			}
			echo_server server;
			server.start(addr);
			return EXIT_SUCCESS;
		}
		if (mode == "client")
		{
			desc.add(client_desc);
			try
			{
				store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
				notify(vm);
			}
			catch (const po::error& ex)
			{
				std::cout << ex.what() << std::endl;
				return EXIT_FAILURE;
			}
			debug_echo_client client;
			client.run(addr, message.data(), message.length(), requests, std::cout);
			return EXIT_SUCCESS;
		}
	}
	desc.add(server_desc).add(client_desc);
	std::cout << desc << std::endl;
	return EXIT_FAILURE;
}
