cmake_minimum_required(VERSION 2.8.7)

if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_C_COMPILER_VERSION VERSION_GREATER 4.9)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.3)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
  endif()
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
endif()

project(myfind)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wstrict-prototypes -pedantic")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")

set(CMAKE_EXE_LINKER_FLAGS="-fprofile-arcs -ftest-coverage")

set(SOURCE_FILES main.c)
add_executable(myfind ${SOURCE_FILES})
