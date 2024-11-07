set(FRONTEND_SOURCES
        include/lexer.h
        src/lexer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../yu/src/tokens.cpp
        ../common/bt.hpp
        include/parser.h
        src/parser.cpp
        src/token_matching.cpp
)

add_library(yu-frontend STATIC ${FRONTEND_SOURCES})

target_include_directories(yu-frontend
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../yu/include
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Set C++ standard
set_target_properties(yu-frontend PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
)

# Clang-specific settings
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_directories(yu-frontend PUBLIC
            /opt/homebrew/opt/llvm/lib
    )

    target_link_libraries(yu-frontend
            PUBLIC
            c++
            c++abi
    )
endif ()
