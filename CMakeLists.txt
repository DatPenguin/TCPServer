cmake_minimum_required(VERSION 3.8)
project(TCPServer)

set(CMAKE_CXX_STANDARD 11)

set(SERVER_FILES
        server/client.h
        server/main.c
        server/server.h)

add_executable(server ${SERVER_FILES})

target_compile_options(server PRIVATE -Wall -Wextra -pedantic)

set(CLIENT_FILES
        client/client.h
        client/main.c)

add_executable(client ${CLIENT_FILES})