add_library(field_io STATIC
    src/fieldReader.cpp
    src/fieldWriter.cpp
)

target_include_directories(field_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(field_io PUBLIC field HighFive)

add_subdirectory(tests)
