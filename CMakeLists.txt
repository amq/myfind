cmake_minimum_required(VERSION 2.8.7)
project(myfind)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -Wextra -Wstrict-prototypes -pedantic -g -O0 -fprofile-arcs -ftest-coverage")

SET(CMAKE_EXE_LINKER_FLAGS="-fprofile-arcs -ftest-coverage")

set(SOURCE_FILES main.c)
add_executable(myfind ${SOURCE_FILES})
