include(FetchContent)

# -- System dependencies -------------------------------------------------------
# Install with: mise run deps

# HDF5 -> binary data format
find_package(HDF5 REQUIRED COMPONENTS C CXX)

# -- FetchContent dependencies -------------------------------------------------

# Eigen -> linear algebra
set(EIGEN_BUILD_DOC     OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_BLAS    OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_LAPACK  OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING       OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    Eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Eigen)

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
