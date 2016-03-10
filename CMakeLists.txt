cmake_minimum_required(VERSION 2.8.7)

project(myfind)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wextra -Wstrict-prototypes -pedantic")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")

set(CMAKE_EXE_LINKER_FLAGS="-fprofile-arcs -ftest-coverage")

set(SOURCE_FILES main.c)
add_executable(myfind ${SOURCE_FILES})
