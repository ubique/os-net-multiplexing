CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

add_library(safe_poll safe_poll.cpp safe_poll.h)
add_library(server Server.cpp Server.h)
target_link_libraries(server safe_poll)

add_executable(start_server startServer.cpp)
target_link_libraries(start_server server)

add_library(client Client.cpp Client.h)
target_link_libraries(client safe_poll)

add_executable(run_client runClient.cpp)
target_link_libraries(run_client client)

add_executable(test test.cpp)
target_link_libraries(test server client -pthread)