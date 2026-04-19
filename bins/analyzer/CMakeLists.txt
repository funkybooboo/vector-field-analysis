# Auto-detect the CUDA toolkit; honour an explicit -DENABLE_CUDA=ON/OFF override.
find_package(CUDAToolkit QUIET)
option(ENABLE_CUDA "Build analyzer CUDA solver" ${CUDAToolkit_FOUND})

if(ENABLE_CUDA)
  # Default covers CHPC GPUs reachable with CUDA 11.5 (the CI toolkit):
  #   sm_60/61 (kingspeak P100, lonepeak/kingspeak 1080Ti/TitanX/P40)
  #   sm_70/75 (notchpeak V100/TitanV, T4/2080Ti)
  #   sm_80/86 (notchpeak A100, A40/A6000/3090)
  # sm_89/90 (granite L40S/H100/H200) require CUDA >= 11.8 and are only
  # reachable on CHPC where native is used anyway.
  # These are compile-check targets; production runs use native:
  #   cmake -B build -DCMAKE_CUDA_ARCHITECTURES=native
  # benchmark.sh and enqueue.sh set native automatically on CHPC nodes.
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 60 61 70 75 80 86)
  endif()
  enable_language(CUDA)
  find_package(CUDAToolkit REQUIRED)

  # FindCUDAToolkit resolves librt via find_library(), which on glibc 2.34+
  # (Ubuntu 22.04+) returns the linker-script stub librt.a -- a text file
  # that nvlink cannot process. Replace it with -lrt so nvlink skips it as
  # a host-only flag. CUDA::cudart (dynamic) also links cudart_static_deps,
  # so this patch is required even when not explicitly linking cudart_static.
  if(UNIX AND NOT APPLE)
    # Strip glibc 2.34+ stub .a files from every CUDA imported target that may
    # carry them. The stubs are host-only linker scripts; nvlink cannot process
    # them and emits "Skipping incompatible" warnings for every architecture.
    # libcudart.so already bakes in the real dl/rt/pthread symbols, so the .a
    # paths are redundant on the device-link line regardless.
    macro(_strip_stub_libs _target)
      get_target_property(_ill ${_target} INTERFACE_LINK_LIBRARIES)
      if(_ill)
        string(REGEX REPLACE "[^;]*/librt\\.a;?"      "" _ill "${_ill}")
        string(REGEX REPLACE "[^;]*/libpthread\\.a;?" "" _ill "${_ill}")
        string(REGEX REPLACE "[^;]*/libdl\\.a;?"      "" _ill "${_ill}")
        string(REGEX REPLACE ";+"                     ";" _ill "${_ill}")
        string(REGEX REPLACE "^;|;$"                  "" _ill "${_ill}")
        set_target_properties(${_target} PROPERTIES INTERFACE_LINK_LIBRARIES "${_ill}")
      endif()
    endmacro()

    foreach(_cuda_tgt IN ITEMS
        CUDA::cudart
        CUDA::cudart_static
        CUDA::cudart_static_deps)
      if(TARGET ${_cuda_tgt})
        _strip_stub_libs(${_cuda_tgt})
      endif()
    endforeach()
  endif()
endif()

add_library(analyzer_lib STATIC
    src/configParser.cpp
    src/hybridCudaMpiStreamlineSolver.cpp
    src/mpiStreamlineSolver.cpp
    src/openMpStreamlineSolver.cpp
    src/pthreadsStreamlineSolver.cpp
    src/runner.cpp
    src/sequentialStreamlineSolver.cpp
    src/solverFactory.cpp
    src/streamlineSolver.cpp
    src/streamWriter.cpp
    src/timing.cpp
)

if(ENABLE_CUDA)
  # Build CUDA kernels in a separate static library and resolve device symbols
  # here, before HDF5 / MPI / OpenMP enter the link closure. This prevents
  # their glibc 2.34+ stub .a files (libpthread.a, libdl.a, librt.a) from
  # being passed to nvlink -- the stub .a files are linker-script stubs that
  # the host linker understands but nvlink cannot process.
  #
  # CUDA kernels only depend on field (for Grid/Vec types) and CUDA::cudart,
  # both of which are stub-free, so the device link step is clean.
  add_library(analyzer_cuda_kernels STATIC
      src/cuda.cu
      src/cudaStreamlineSolver.cu
  )
  target_include_directories(analyzer_cuda_kernels PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  target_link_libraries(analyzer_cuda_kernels PUBLIC CUDA::cudart PRIVATE field)
  set_target_properties(analyzer_cuda_kernels PROPERTIES
      CUDA_SEPARABLE_COMPILATION ON
      CUDA_RESOLVE_DEVICE_SYMBOLS ON
      CUDA_STANDARD 17
      CUDA_STANDARD_REQUIRED ON
  )
  target_compile_options(analyzer_cuda_kernels PRIVATE
      $<$<COMPILE_LANGUAGE:CUDA>:-allow-unsupported-compiler>
  )
  target_compile_definitions(analyzer_cuda_kernels PUBLIC ENABLE_CUDA_SOLVER=1)
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
  target_link_libraries(analyzer_lib PUBLIC analyzer_cuda_kernels)
endif()

find_package(MPI)
if(MPI_FOUND)
    # USE_MPI must be PUBLIC so main.cpp (which links analyzer_lib) sees the definition.
    target_compile_definitions(analyzer_lib PUBLIC USE_MPI)
    target_link_libraries(analyzer_lib PUBLIC MPI::MPI_CXX)
endif()

find_package(OpenMP)
if(OpenMP_FOUND)
    target_link_libraries(analyzer_lib PUBLIC OpenMP::OpenMP_CXX)
endif()

add_executable(analyzer src/main.cpp)
target_link_libraries(analyzer PRIVATE
                      analyzer_lib
                      utils
                      stdc++fs
                     )

add_subdirectory(tests)
