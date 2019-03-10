cmake_minimum_required(VERSION 3.1)
project(librcnb C)

include_directories(include)

add_executable(main src/cencode.c)
add_executable(main2 src/cdecode.c)
