cmake_minimum_required(VERSION 3.21...3.31)

project(
    FOORT
    VERSION 1.0
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/FOORT/bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "-std=c++17 -fopenmp -O3 -Wno-unused-result")
set(CMAKE_EXE_LINKER_FLAGS "-lm -lstdc++fs")

include(Doxygen)
include(Testing)
#include(ClangFormat)

Doxygen("FOORT/src" docs)

add_subdirectory(FOORT)
