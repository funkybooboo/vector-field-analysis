add_executable(simulator_tests
    test_simulator.cpp
)

target_link_libraries(simulator_tests PRIVATE vector Eigen3::Eigen Catch2::Catch2WithMain)

catch_discover_tests(simulator_tests)
