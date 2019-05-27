cmake_minimum_required(VERSION 3.10)
project(os-net-multiplexing)

set(CMAKE_CXX_STANDARD 14)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(server server/main.cpp util/EventManager.cpp)
add_executable(client client/main.cpp util/EventManager.cpp)