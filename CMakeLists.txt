cmake_minimum_required(VERSION 3.12)
project(Task6)

set(CMAKE_CXX_STANDARD 14)

add_executable(Task6-client client.cpp utils.h utils.cpp)
add_executable(Task6-server server.cpp utils.h utils.cpp)