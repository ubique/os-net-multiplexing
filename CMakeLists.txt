cmake_minimum_required(VERSION 3.0)
project(os_net_multiplexing)

set(CMAKE_CXX_STANDARD 14)

add_executable(os_net_server Server.cpp Server.h server_main.cpp)

add_executable(os_net_client client_main.cpp Client.cpp Client.h)