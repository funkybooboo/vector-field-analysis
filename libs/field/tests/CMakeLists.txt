add_executable(field_tests
    test_vec2.cpp
    test_streamline.cpp
    test_grid.cpp
)

target_link_libraries(field_tests PRIVATE field Catch2::Catch2WithMain)

catch_discover_tests(field_tests)
