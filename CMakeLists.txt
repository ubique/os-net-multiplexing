cmake_minimum_required(VERSION 2.0)
project(OS_net_multiplexing)

set(CMAKE_CXX_STANDARD 14)

add_executable(client client.cpp)
add_executable(server server.cpp)