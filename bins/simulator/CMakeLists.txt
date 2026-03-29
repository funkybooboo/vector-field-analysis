add_library(simulator_lib STATIC
    src/configParser.cpp
    src/fieldGenerator.cpp
    src/fieldWriter.cpp
)

target_include_directories(simulator_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(simulator_lib PUBLIC
    Eigen3::Eigen
    exprtk
    stb_perlin
    HighFive
    tomlplusplus::tomlplusplus
)

add_executable(simulator src/main.cpp)

target_link_libraries(simulator PRIVATE simulator_lib)

add_subdirectory(tests)
