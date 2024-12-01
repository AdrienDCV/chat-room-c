cmake_minimum_required(VERSION 3.29)
project(chat_room_c C)

set(CMAKE_C_STANDARD 11)

add_executable(chat_room_c
    server.c
        client.c)
