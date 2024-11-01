# This file is part of the Yu programming language and is licensed under MIT License;
# See LICENSE.txt for details

# Move googletest setup before the executable definition
# This ensures GTest targets are available when we need them
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/googletest ${CMAKE_BINARY_DIR}/googletest)

# Create the test executable
add_executable(yu-test
        unittest/tokenizing.cpp
)

# Include directories - keeping the essential ones
target_include_directories(yu-test PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/frontend/include
        ${CMAKE_SOURCE_DIR}/vendor/googletest/googletest/include
)

# Specify proper link directories for LLVM
if(APPLE)
    target_link_directories(yu-test PRIVATE
            /opt/homebrew/opt/llvm/lib
    )
endif()

# Link against libraries - ensure yu-frontend is built before linking
add_dependencies(yu-test yu-frontend)

target_link_libraries(yu-test PRIVATE
        yu-frontend
        GTest::gtest
        GTest::gtest_main
)

# For Clang/LLVM on Apple platforms
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(yu-test PRIVATE
            c++
            c++abi
    )
endif()

# Set C++20 standard
set_target_properties(yu-test PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# Enable testing
enable_testing()
include(GoogleTest)
gtest_discover_tests(yu-test
        PROPERTIES
        ENVIRONMENT "GTEST_COLOR=yes"
)