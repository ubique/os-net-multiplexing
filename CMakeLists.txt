cmake_minimum_required(VERSION 2.8)

project(os_net_multiplex)

include_directories(client common server multiplex)

add_library(libnetw client/Client.cpp multiplex/EpollMultiplexor.cpp multiplex/SelectMultiplexor.cpp server/Acceptor.cpp server/Server.cpp)

add_executable(ClientMain ClientMain.cpp)
target_link_libraries(ClientMain libnetw)
add_executable(ServerMain ServerMain.cpp)
target_link_libraries(ServerMain libnetw)
