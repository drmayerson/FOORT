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

# Append to (rather than overwrite) user-supplied flags; OpenMP comes from the OpenMP::OpenMP_CXX target
# (FOORT/src/CMakeLists.txt), and Release uses CMake's default optimisation flags (-O3 -DNDEBUG)
add_compile_options(-Wno-unused-result)

option(FOORT_BUILD_TESTS "Build the FOORT test suite (fetches GoogleTest)" ON)

include(Filesystem)

# Checked here (rather than only in FOORT/src) so a missing OpenMP is reported before GoogleTest is fetched
find_package(OpenMP)
if(NOT OpenMP_CXX_FOUND)
    set(OPENMP_MESSAGE "OpenMP was not found for ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER}).")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        string(APPEND OPENMP_MESSAGE " Apple Clang does not support OpenMP: run `source alternative_builds/MacOS_workaround.sh` "
            "(Homebrew GCC), rm -rf build, and reconfigure (see 'MacOS Troubleshooting' in README.md).")
    endif()
    message(FATAL_ERROR "${OPENMP_MESSAGE}")
endif()

include(Doxygen)
if(FOORT_BUILD_TESTS)
    include(Testing)
endif()
#include(ClangFormat)

Doxygen("FOORT/src" docs)

add_subdirectory(FOORT)
