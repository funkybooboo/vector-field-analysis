include(FetchContent)

# -- System dependencies -------------------------------------------------------
# Install with: mise run deps
# Tested versions (Ubuntu 22.04):
#   libhdf5-dev       1.10.7+repack-4ubuntu2
#   libopenmpi-dev    4.1.2-2ubuntu1
#   nvidia-cuda-toolkit 11.5.1-1ubuntu1

# HDF5 -> binary data format
find_package(HDF5 1.10 REQUIRED COMPONENTS C CXX)

# -- FetchContent dependencies -------------------------------------------------

# HighFive -> HDF5 C++ wrapper
set(HIGHFIVE_USE_BOOST  OFF CACHE BOOL "" FORCE)
set(HIGHFIVE_USE_EIGEN  OFF CACHE BOOL "" FORCE)
set(HIGHFIVE_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(HIGHFIVE_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(HIGHFIVE_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    HighFive
    GIT_REPOSITORY https://github.com/BlueBrain/HighFive.git
    GIT_TAG        v2.10.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(HighFive)
set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)

# Catch2 -> testing
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

# toml++ -> TOML config file parsing
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(tomlplusplus)

# exprtk -> math expression parsing for custom fields
FetchContent_Declare(
    exprtk
    GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git
    GIT_TAG        0.0.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(exprtk)
add_library(exprtk INTERFACE)
target_include_directories(exprtk INTERFACE ${exprtk_SOURCE_DIR})

# stb_perlin -> Perlin noise for noise field type
# stb has no release tags; pinned to a specific commit instead.
FetchContent_Declare(
    stb_perlin
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        31c1ad37456438565541f4919958214b6e762fb4
)
FetchContent_MakeAvailable(stb_perlin)
add_library(stb_perlin INTERFACE)
target_include_directories(stb_perlin INTERFACE ${stb_perlin_SOURCE_DIR})
