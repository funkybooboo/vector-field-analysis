add_library(analyzer_lib STATIC
    src/vectorField.cpp
    src/fieldReader.cpp
    src/openMP.cpp
    src/pthreads.cpp
    src/sequentialCPU.cpp
)

target_include_directories(analyzer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(analyzer_lib PUBLIC vector HighFive)

add_executable(analyzer src/main.cpp)

target_link_libraries(analyzer PRIVATE analyzer_lib)

add_subdirectory(tests)
