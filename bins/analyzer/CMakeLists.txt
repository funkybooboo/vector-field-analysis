add_library(analyzer_lib STATIC
    src/analyzerConfigParser.cpp
    src/mpiStreamlineSolver.cpp
    src/openMPStreamlineSolver.cpp
    src/pthreadsStreamlineSolver.cpp
    src/sequentialStreamlineSolver.cpp
    src/solverFactory.cpp
    src/streamWriter.cpp
)

target_include_directories(analyzer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(analyzer_lib PUBLIC field field_io utils HighFive tomlplusplus::tomlplusplus)

find_package(MPI)
if(MPI_FOUND)
    target_compile_definitions(analyzer_lib PUBLIC USE_MPI)
    target_link_libraries(analyzer_lib PUBLIC MPI::MPI_CXX)
endif()

add_executable(analyzer src/main.cpp)

target_link_libraries(analyzer PRIVATE analyzer_lib)

add_subdirectory(tests)
