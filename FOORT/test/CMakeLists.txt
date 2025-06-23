add_executable(test_integrator test_integrator.cpp)
add_executable(test_einstein_ring test_einstein_ring.cpp)
add_executable(test_rot_bos test_rot_bos.cpp)

target_include_directories(test_integrator PUBLIC ${CMAKE_SOURCE_DIR}/FOORT/src)
target_include_directories(test_einstein_ring PUBLIC ${CMAKE_SOURCE_DIR}/FOORT/src)
target_include_directories(test_rot_bos PUBLIC ${CMAKE_SOURCE_DIR}/FOORT/src)

target_link_libraries(test_integrator PUBLIC config_lib metric_lib input_output_lib GTest::gtest_main)
target_link_libraries(test_einstein_ring PUBLIC config_lib metric_lib input_output_lib GTest::gtest_main)
target_link_libraries(test_rot_bos PUBLIC config_lib metric_lib input_output_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_integrator test_einstein_ring test_rot_bos)
