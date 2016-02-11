cmake_minimum_required(VERSION 3.2)
project(myfind)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 \
-Wall -Wextra -Wstrict-prototypes -pedantic \
-g -O0 -fprofile-arcs -ftest-coverage")

SET(CMAKE_EXE_LINKER_FLAGS="-fprofile-arcs -ftest-coverage")

set(SOURCE_FILES main.c lib.h lib.c)
add_executable(myfind ${SOURCE_FILES})