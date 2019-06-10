cmake_minimum_required(VERSION 3.0)
project(CLIENT_SERVER)

set(CMAKE_CXX_STANDARD 14)

add_executable(client client.cpp Utils.h)
add_executable(server server.cpp Utils.h)