add_executable(analyzer_tests
    test_vectorfield.cpp
    test_fieldReader.cpp
)

target_link_libraries(analyzer_tests PRIVATE analyzer_lib Catch2::Catch2WithMain)

catch_discover_tests(analyzer_tests)
