cmake_minimum_required(VERSION 3.10)
project(echo)

set(CMAKE_CXX_STANDARD 17)

add_executable(echo epoll.h epoll.cpp unix_base_socket.h unix_base_socket.cpp unix_socket.h unix_socket.cpp unix_server_socket.h unix_server_socket.cpp echo_server.h echo_server.cpp debug_echo_client.h debug_echo_client.cpp main.cpp)

target_link_libraries(echo stdc++fs pthread)

find_package(Boost REQUIRED COMPONENTS program_options)

include_directories(${Boost_INCLUDE_DIR})

target_link_libraries(echo ${Boost_LIBRARIES}) 
