cmake_minimum_required(VERSION 3.31)
project(dom_lib C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(src)
add_subdirectory(tests)
