cmake_minimum_required(VERSION 3.12)

project(tftp)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address,undefined")

add_library(common
        common.h common.c
        utils.h utils.c)

add_executable(run-server run-server.c)
add_executable(run-client run-client.c)

target_link_libraries(run-server common)
target_link_libraries(run-client common)

