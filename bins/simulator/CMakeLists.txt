add_library(simulator_lib STATIC
    src/configParser.cpp
    src/fieldGenerator.cpp
)

target_include_directories(simulator_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(simulator_lib
    PUBLIC  field field_io config
    PRIVATE utils exprtk stb_perlin tomlplusplus::tomlplusplus
)

add_executable(simulator src/main.cpp)

target_link_libraries(simulator PRIVATE simulator_lib utils)

add_subdirectory(tests)
