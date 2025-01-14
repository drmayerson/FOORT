add_executable(test_integrator test_integrator.cpp)

target_include_directories(test_integrator PUBLIC ${CMAKE_SOURCE_DIR}/FOORT/src)

target_link_libraries(test_integrator PUBLIC config_lib metric_lib input_output_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_integrator)
