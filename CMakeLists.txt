cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 14)

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

project(sntp-multiplexing)

add_library(multiplexer STATIC multiplexer.cpp
                               multiplexer.h)

add_executable(sntp-client-multiplexing
               client.cpp
               client.h
               protocol.h)

target_link_libraries(sntp-client-multiplexing
                      multiplexer)

add_executable(sntp-server-multiplexing
               server.cpp
               server.h
               protocol.h)

target_link_libraries(sntp-server-multiplexing
                      multiplexer)

install(TARGETS sntp-client-multiplexing
                sntp-server-multiplexing
        RUNTIME DESTINATION bin)
