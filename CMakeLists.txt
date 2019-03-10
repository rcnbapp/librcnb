cmake_minimum_required(VERSION 3.1)
project(librcnb C)

include_directories(include)

add_library(rcnb SHARED src/cencode.c src/cdecode.c src/rcnb.c)
