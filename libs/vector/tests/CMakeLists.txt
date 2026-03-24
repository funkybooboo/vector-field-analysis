add_executable(vector_tests
    test_vector.cpp
    test_streamline.cpp
)

target_link_libraries(vector_tests PRIVATE vector Catch2::Catch2WithMain)

catch_discover_tests(vector_tests)
