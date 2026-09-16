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

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-fopenmp -Wno-unused-result")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_EXE_LINKER_FLAGS "-lm -lstdc++fs")

option(FOORT_BUILD_TESTS "Build the FOORT test suite (fetches GoogleTest)" ON)

include(Doxygen)
if(FOORT_BUILD_TESTS)
    include(Testing)
endif()
#include(ClangFormat)

Doxygen("FOORT/src" docs)

add_subdirectory(FOORT)
