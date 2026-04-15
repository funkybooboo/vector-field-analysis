add_executable(simulator_tests
    test_simulator.cpp
)

target_include_directories(simulator_tests PRIVATE ${CMAKE_SOURCE_DIR}/bins/tests)
target_link_libraries(simulator_tests PRIVATE simulator_lib Catch2::Catch2WithMain)

catch_discover_tests(simulator_tests)
