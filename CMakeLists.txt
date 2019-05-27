cmake_minimum_required(VERSION 2.8)

project(net)

set(CMAKE_CXX_STANDARD 14)

add_executable(client descriptor.h client.cpp)
add_executable(server descriptor.h server.cpp)
