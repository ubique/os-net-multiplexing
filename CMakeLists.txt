cmake_minimum_required(VERSION 2.8)
project(os-net-multiplexing)

set(CMAKE_CXX_STANDARD 14)

add_executable(server server.cpp)
add_executable(client client.cpp)