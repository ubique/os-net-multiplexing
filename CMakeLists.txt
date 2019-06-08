cmake_minimum_required(VERSION 3.9)

project(os-net-multiplexing)

add_executable(server main_server.cpp Server.cpp Epoll.cpp Socket.cpp)

add_executable(client main_client.cpp Client.cpp Epoll.cpp Socket.cpp)