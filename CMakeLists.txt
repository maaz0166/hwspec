cmake_minimum_required(VERSION 3.20)
project(hwspec VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HWSPEC_BUILD_TESTS "Build the tests" ON)

# Strict warnings for every target.
function(hwspec_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

# Core library: all the real work lives here. Collectors will be added under core/src/windows/.
add_library(hwspec_core STATIC core/src/version.cpp)
target_include_directories(hwspec_core PUBLIC core/include)
target_compile_definitions(hwspec_core PRIVATE HWSPEC_VERSION="${PROJECT_VERSION}")
hwspec_warnings(hwspec_core)

# Command-line program: a thin layer over the core library.
add_executable(hwspec cli/main.cpp)
target_link_libraries(hwspec PRIVATE hwspec_core)
hwspec_warnings(hwspec)

if(HWSPEC_BUILD_TESTS)
  enable_testing()
  add_executable(test_version tests/test_version.cpp)
  target_link_libraries(test_version PRIVATE hwspec_core)
  target_compile_definitions(test_version PRIVATE EXPECTED_VERSION="${PROJECT_VERSION}")
  hwspec_warnings(test_version)
  add_test(NAME version COMMAND test_version)
endif()
