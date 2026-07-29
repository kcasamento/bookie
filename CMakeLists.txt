cmake_minimum_required(VERSION 3.21)
project(hft_orderbook CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)

include(CTest)   # enables testing() + registers the `ctest` driver project-wide

add_subdirectory(exercises)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(benchmark)
