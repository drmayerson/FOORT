add_executable(FOORT Main.cpp Geometry.h)

# ClangFormat(play_simple .)

add_library(config_lib STATIC Config.cpp Config.h Geometry.h)
add_library(config_reader_lib STATIC ConfigReader.cpp ConfigReader.h)
add_library(diagnostics_lib STATIC Diagnostics.cpp Diagnostics.h Geometry.h)
add_library(diagnostics_emission_lib STATIC DiagnosticsEmission.cpp DiagnosticsEmission.h Geometry.h)
add_library(geodesic_lib STATIC Geodesic.cpp Geodesic.h Geometry.h)
add_library(grid_lib STATIC Grid.cpp Grid.h)
add_library(interpolator_lib STATIC Interpolator.cpp Interpolator.h)
add_library(input_output_lib STATIC InputOutput.cpp InputOutput.h Geometry.h)
add_library(integrators_lib STATIC Integrators.cpp Integrators.h Geometry.h)
add_library(mesh_lib STATIC Mesh.cpp Mesh.h Geometry.h)
add_library(metric_lib STATIC Metric.cpp Metric.h Geometry.h)
add_library(spline_lib STATIC Spline.cpp Spline.h)
add_library(terminations_lib STATIC Terminations.cpp Terminations.h Geometry.h)
add_library(utilities_lib STATIC Utilities.cpp Utilities.h)
add_library(viewscreen_lib STATIC ViewScreen.cpp ViewScreen.h Geometry.h)

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

# Expose the result as a small INTERFACE library and link it directly (and last) onto every
# final executable below, rather than relying on it propagating transitively through the rest
# of this file's cyclic static-library dependency graph. With GNU ld's single-pass archive
# resolution (older binutils, e.g. on HPC clusters with GCC < 9), a library attached only via
# an intermediate static lib's PUBLIC requirement can fail to reach the right place on the
# final link line; a direct, explicitly-last dependency of the executable itself cannot.
add_library(filesystem_compat INTERFACE)
if(FILESYSTEM_LINKS_WITH_STDCXXFS)
    target_link_libraries(filesystem_compat INTERFACE stdc++fs)
endif()

target_link_libraries(FOORT PRIVATE config_lib diagnostics_lib geodesic_lib input_output_lib integrators_lib metric_lib terminations_lib utilities_lib viewscreen_lib filesystem_compat)
target_link_libraries(config_lib PUBLIC diagnostics_lib geodesic_lib integrators_lib metric_lib terminations_lib viewscreen_lib config_reader_lib)
target_link_libraries(diagnostics_lib PUBLIC diagnostics_emission_lib geodesic_lib input_output_lib integrators_lib metric_lib)
target_link_libraries(diagnostics_emission_lib PUBLIC input_output_lib integrators_lib metric_lib)
target_link_libraries(geodesic_lib PUBLIC diagnostics_lib input_output_lib integrators_lib metric_lib terminations_lib)
target_link_libraries(integrators_lib PUBLIC geodesic_lib metric_lib)
target_link_libraries(mesh_lib PUBLIC diagnostics_lib input_output_lib utilities_lib)
target_link_libraries(metric_lib PUBLIC grid_lib input_output_lib integrators_lib interpolator_lib spline_lib)
target_link_libraries(terminations_lib PUBLIC geodesic_lib input_output_lib)
target_link_libraries(utilities_lib PUBLIC diagnostics_lib geodesic_lib integrators_lib metric_lib terminations_lib viewscreen_lib)
target_link_libraries(viewscreen_lib PUBLIC metric_lib mesh_lib)

find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(FOORT PUBLIC OpenMP::OpenMP_CXX)
endif()
