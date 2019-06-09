cmake_minimum_required(VERSION 2.8)

project(os-net)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined")

include_directories(${CMAKE_SOURCE_DIR})

add_library(lib
        lib/epoll_wrapper.h
        lib/epoll_wrapper.cpp
        lib/error.h
        lib/socket_wrapper.h
        lib/socket_wrapper.cpp)

add_executable(os-net-server
        server/main.cpp
        server/server.h
        server/server.cpp)

add_executable(os-net-client
        client/main.cpp
        client/client.h
        client/client.cpp)

target_link_libraries(os-net-server lib)
target_link_libraries(os-net-client lib)
