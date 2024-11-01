# This file is part of the Yu programming language and is licensed under MIT License;
# See LICENSE.txt for details

cmake_minimum_required(VERSION 3.29)

add_executable(yu-cli
        src/yu-cli.cpp
        ../common/allocator.cpp
)

target_include_directories(yu-cli PRIVATE
        include
)

set_target_properties(yu-cli
        PROPERTIES
        CXX_STANDARD 17
)

target_link_libraries(yu-cli PRIVATE
        stdc++
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(yu-cli PRIVATE c++abi)
endif()