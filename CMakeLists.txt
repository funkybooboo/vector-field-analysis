cmake_minimum_required(VERSION 3.18)

project(vector_field_analysis
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
  "$<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Wpedantic;-Werror>"
  "$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic;-Werror>"
)

include(cmake/Dependencies.cmake)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

enable_testing()
include(CTest)
include(Catch)
set(CMAKE_CATCH_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

add_subdirectory(libs/field)
add_subdirectory(libs/utils)
add_subdirectory(libs/field_io)
add_subdirectory(bins/analyzer)
add_subdirectory(bins/simulator)
