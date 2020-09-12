cmake_minimum_required(VERSION 3.5)

project(sockets_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sockets_test main.cpp)

target_link_libraries(sockets_test Ws2_32)
