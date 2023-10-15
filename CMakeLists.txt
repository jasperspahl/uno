cmake_minimum_required(VERSION 3.26)
project(uno C)

set(CMAKE_C_STANDARD 17)

find_package(Threads REQUIRED)

# add pthreads to uno_server
add_executable(uno_server
        server.c
        server.h
        types.h
        common.c
        common.h)
target_link_libraries(uno_server Threads::Threads)


add_executable(uno_client client.c client.h types.h
        common.c
        common.h)
target_link_libraries(uno_client Threads::Threads)
