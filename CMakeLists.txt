cmake_minimum_required(VERSION 2.8)

project(os-net-multiplexing)

include_directories(${PROJECT_SOURCE_DIR})

add_library(socket "socket/socket_wrapper.h" "socket/socket_wrapper.cpp")
add_library(server-lib "server/server.h" "server/server.cpp")
add_library(client-lib "client/client.h" "client/client.cpp")
add_library(epoll-util "epolling.h" "epolling.cpp")

add_executable(client "client_main.cpp")
add_executable(server "server_main.cpp")

target_link_libraries(client client-lib socket epoll-util)
target_link_libraries(server server-lib socket epoll-util)
