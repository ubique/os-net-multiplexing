cmake_minimum_required(VERSION 2.8.11)
project(multiplexing)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -D_GLIBCXX_DEBUG ")
endif ()


add_executable(multiplexing-server Epoll.cpp Epoll.h helper.cpp helper.h Socket.cpp Socket.h main-server.cpp)

add_executable(multiplexing-client Epoll.cpp Epoll.h helper.cpp helper.h Socket.cpp Socket.h main-client.cpp)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")