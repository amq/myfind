cmake_minimum_required(VERSION 2.8.7)
find_package(Doxygen)

project(myfind)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wextra -Wstrict-prototypes -pedantic")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")

set(CMAKE_EXE_LINKER_FLAGS="-fprofile-arcs -ftest-coverage")

set(SOURCE_FILES main.c)
add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

if(DOXYGEN_FOUND)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${DOXYGEN_EXECUTABLE})
endif()
