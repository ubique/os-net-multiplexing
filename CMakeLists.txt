cmake_minimum_required(VERSION 3.10)

project(net-multiplexing)

add_library(utilslib
        utils/logger.cpp utils/logger.hpp
        utils/fd_wrapper.cpp utils/fd_wrapper.hpp)

add_executable(client client.hpp client.cpp)
add_executable(server server.hpp server.cpp)
target_link_libraries(client utilslib)
target_link_libraries(server utilslib)