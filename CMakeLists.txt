cmake_minimum_required(VERSION 3.12)
project(Task6)

set(CMAKE_CXX_STANDARD 14)

add_executable(Task6_client client.cpp utils.cpp)
add_executable(Task6_server server.cpp utils.cpp)