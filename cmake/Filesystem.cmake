# Included from the top-level CMakeLists.txt before Testing.cmake, so that an unsuitable compiler is
# reported before any time is spent fetching GoogleTest (which may stall on offline HPC login nodes).
# Defines FILESYSTEM_LINKS_WITHOUT_EXTRA_LIB / FILESYSTEM_LINKS_WITH_STDCXXFS, used in FOORT/src.
#
# input_output_lib uses std::filesystem::create_directories (InputOutput.cpp). On some toolchains
# (e.g. GCC < 9) this needs linking against a separate libstdc++fs; on others (Apple Clang/libc++,
# GCC >= 9) it's already part of the standard library and an explicit -lstdc++fs can fail to link
# (or even break unrelated targets, since it doesn't exist on Apple's toolchain at all). Detect
# which case we're in instead of assuming either way.
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_QUIET TRUE)
# Force C++17 explicitly for this check: CMAKE_CXX_STANDARD is not guaranteed to propagate into
# check_cxx_source_compiles's own try_compile on every CMake/compiler combination, and without it
# <filesystem> can fail to even compile on GCC's pre-17 default dialect (e.g. GCC 8's gnu++14) --
# which looks identical to a link failure from this check's perspective and defeats the detection.
set(CMAKE_REQUIRED_FLAGS "-std=c++17")
check_cxx_source_compiles(
    "#include <filesystem>
    int main() { std::filesystem::create_directories(std::filesystem::path(\".\")); return 0; }"
    FILESYSTEM_LINKS_WITHOUT_EXTRA_LIB
)
if(NOT FILESYSTEM_LINKS_WITHOUT_EXTRA_LIB)
    set(CMAKE_REQUIRED_LIBRARIES stdc++fs)
    check_cxx_source_compiles(
        "#include <filesystem>
        int main() { std::filesystem::create_directories(std::filesystem::path(\".\")); return 0; }"
        FILESYSTEM_LINKS_WITH_STDCXXFS
    )
    unset(CMAKE_REQUIRED_LIBRARIES)
endif()
unset(CMAKE_REQUIRED_FLAGS)
unset(CMAKE_REQUIRED_QUIET)

# Neither check passing almost always means a standard library without <filesystem> (e.g. GCC < 8),
# which would otherwise only surface as a "No such file or directory" error midway through the build.
# Fail early with a clear message; the override exists in case this check misfires on a toolchain
# that can in fact build the code.
option(FOORT_SKIP_FILESYSTEM_CHECK "Downgrade the configure-time std::filesystem check to a warning" OFF)
if(NOT FILESYSTEM_LINKS_WITHOUT_EXTRA_LIB AND NOT FILESYSTEM_LINKS_WITH_STDCXXFS)
    string(CONCAT FILESYSTEM_CHECK_MESSAGE
        "Could not compile and link a C++17 std::filesystem test program with "
        "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER}). "
        "FOORT needs a compiler whose standard library provides <filesystem>, e.g. GCC >= 8. "
        "Load a newer compiler (e.g. `module load gcc/<version>` on a cluster) and reconfigure in a "
        "fresh build directory (rm -rf build). If you believe this check is wrong, see the configure "
        "log under the build directory's CMakeFiles/ for details, and pass "
        "-DFOORT_SKIP_FILESYSTEM_CHECK=ON to proceed anyway.")
    # Apple Clang fails this check because it rejects the global -fopenmp flag, not because
    # <filesystem> is missing; point macOS users at the Homebrew GCC workaround instead.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        string(CONCAT FILESYSTEM_CHECK_MESSAGE
            "${FILESYSTEM_CHECK_MESSAGE} "
            "NOTE: you are using Apple Clang, which does not support -fopenmp; this is most likely "
            "the actual cause. Run `source alternative_builds/MacOS_workaround.sh` (Homebrew GCC), "
            "rm -rf build, and reconfigure (see 'MacOS Troubleshooting' in README.md).")
    endif()
    if(FOORT_SKIP_FILESYSTEM_CHECK)
        message(WARNING "${FILESYSTEM_CHECK_MESSAGE}")
    else()
        message(FATAL_ERROR "${FILESYSTEM_CHECK_MESSAGE}")
    endif()
endif()

