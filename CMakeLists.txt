cmake_minimum_required(VERSION 3.16)
project(a2 C)

set(CMAKE_C_STANDARD 99)

add_executable(a2 a2.c a2_helper.c a2_helper.h)
target_link_libraries(a2 pthread)