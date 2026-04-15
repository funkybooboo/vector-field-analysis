add_library(analyzer_lib STATIC
    src/configParser.cpp
    src/mpiStreamlineSolver.cpp
    src/openMpStreamlineSolver.cpp
    src/pthreadsStreamlineSolver.cpp
    src/sequentialStreamlineSolver.cpp
    src/solverFactory.cpp
    src/streamWriter.cpp
)

target_include_directories(analyzer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(analyzer_lib
    PUBLIC  field field_io config
    PRIVATE utils HighFive tomlplusplus::tomlplusplus
)

find_package(MPI)
if(MPI_FOUND)
    # USE_MPI must be PUBLIC so main.cpp (which links analyzer_lib) sees the definition.
    target_compile_definitions(analyzer_lib PUBLIC USE_MPI)
    target_link_libraries(analyzer_lib PUBLIC MPI::MPI_CXX)
endif()

find_package(OpenMP)
if(OpenMP_FOUND)
    target_link_libraries(analyzer_lib PRIVATE OpenMP::OpenMP_CXX)
endif()

add_executable(analyzer src/main.cpp)

target_link_libraries(analyzer PRIVATE analyzer_lib utils)

add_subdirectory(tests)
