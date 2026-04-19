add_library(field STATIC
    src/vec2.cpp
    src/grid.cpp
)

target_include_directories(field PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_subdirectory(tests)
