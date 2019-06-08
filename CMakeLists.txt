cmake_minimum_required(VERSION 3.10)
project(os-net-multiplexing)

set(CMAKE_CXX_STANDARD 14)

add_executable(os-net-multiplexing-server server.cpp utils.cpp utils.h)
add_executable(os-net-multiplexing-client client.cpp utils.cpp utils.h)
