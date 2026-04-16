option(ENABLE_CUDA "Build analyzer CUDA solver" OFF)

if(ENABLE_CUDA)
  set(CMAKE_CUDA_ARCHITECTURES 52)
  enable_language(CUDA)
  find_package(CUDAToolkit REQUIRED)
endif()

add_library(analyzer_lib STATIC
    src/configParser.cpp
    src/mpiStreamlineSolver.cpp
    src/openMpStreamlineSolver.cpp
    src/pthreadsStreamlineSolver.cpp
    src/sequentialStreamlineSolver.cpp
    src/solverFactory.cpp
    src/streamWriter.cpp
)

if(ENABLE_CUDA)
  target_sources(analyzer_lib PRIVATE
                 src/cudaStreamlineSolver.cu
                 src/cudaFull.cu 
                 src/cudaFullStreamlineSolver.cu
               )
endif()

target_include_directories(analyzer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(analyzer_lib 
  PUBLIC  
    field 
    field_io
    HighFive
                      
  PRIVATE 
    utils
    tomlplusplus::tomlplusplus
)

if(ENABLE_CUDA)
  target_link_libraries(analyzer_lib PUBLIC
                        CUDA::cudart 
  )
  target_compile_definitions(analyzer_lib PUBLIC
                             ENABLE_CUDA_SOLVER=1
  )
  set_target_properties(analyzer_lib PROPERTIES 
                        CUDA_SEPARABLE_COMPILATION ON
                        CUDA_STANDARD 17
                        CUDA_STANDARD_REQUIRED ON
  )
endif()

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
target_link_libraries(analyzer PRIVATE 
                      analyzer_lib 
                      utils
                      stdc++fs 
                     )

add_subdirectory(tests)
