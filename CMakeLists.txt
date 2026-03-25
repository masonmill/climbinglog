cmake_minimum_required(VERSION 3.20)
project(climbinglog)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_subdirectory(core)

enable_testing()
add_subdirectory(test)
