add_library(vector STATIC
    src/vector.cpp
    src/streamline.cpp
)

target_include_directories(vector PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_subdirectory(tests)
