cmake_minimum_required(VERSION 3.22.1)

project(zasm C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -fsanitize=address")

file (GLOB_RECURSE source_files ${PROJECT_SOURCE_DIR}/*.c)
file (GLOB_RECURSE header_files ${PROJECT_SOURCE_DIR}/*.h)

add_executable(zasm ${header_files} ${source_files})

