cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 14)

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
