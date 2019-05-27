cmake_minimum_required(VERSION 3.0)
project(os_net_multiplexing)

set(CMAKE_CXX_STANDARD 14)



add_executable(os-net-multiplexing-client
        Client/Client.cpp
        Client/Client.h
        Client/ClientException.h
        )

add_executable(os-net-multiplexing-server
        Server/Server.cpp
        Server/Server.h
        Server/ServerException.h
        )

install(TARGETS os-net-multiplexing-client
        os-net-multiplexing-server
        RUNTIME DESTINATION bin
        )