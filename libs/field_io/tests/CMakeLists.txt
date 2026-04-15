add_executable(field_io_tests
    test_fieldReader.cpp
    test_fieldWriter.cpp
)

target_link_libraries(field_io_tests PRIVATE field_io Catch2::Catch2WithMain)

catch_discover_tests(field_io_tests)
