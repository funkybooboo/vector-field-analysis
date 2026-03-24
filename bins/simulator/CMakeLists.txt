add_executable(simulator src/main.cpp)

target_link_libraries(simulator PRIVATE vector Eigen3::Eigen HighFive)

add_subdirectory(tests)
