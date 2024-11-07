# This file is part of the Yu programming language and is licensed under MIT License;
# See LICENSE.txt for details

cmake_minimum_required(VERSION 3.29)
project(Yu LANGUAGES CXX)

option(YU_BUILD_TESTS "Build tests" ON)
option(YU_BUILD_BENCHMARKS "Build benchmarks" OFF)

cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0091 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(cli)
add_subdirectory(frontend)

if (YU_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

if (YU_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif ()