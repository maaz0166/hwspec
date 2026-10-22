cmake_minimum_required(VERSION 3.20)
project(hwspec VERSION 0.3.0 LANGUAGES CXX)

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

# ---- Core library: all the real work (collectors, formatting, report) ----
set(HWSPEC_CORE_SOURCES
  core/src/version.cpp
  core/src/format.cpp
  core/src/json.cpp
  core/src/report.cpp
  core/src/sections.cpp)

if(WIN32)
  # The real collectors, one file per section, plus a small registry helper.
  list(APPEND HWSPEC_CORE_SOURCES
    core/src/windows/registry.cpp
    core/src/windows/cpu.cpp
    core/src/windows/mem.cpp
    core/src/windows/os.cpp
    core/src/windows/gpu.cpp
    core/src/windows/disk.cpp
    core/src/windows/net.cpp
    core/src/windows/battery.cpp
    core/src/windows/wmi.cpp)
else()
  # Not Windows: a stub that returns empty data, so the rest can still be built and tested.
  list(APPEND HWSPEC_CORE_SOURCES core/src/stub/collectors_stub.cpp)
endif()

add_library(hwspec_core STATIC ${HWSPEC_CORE_SOURCES})
target_include_directories(hwspec_core PUBLIC core/include)
target_compile_definitions(hwspec_core PRIVATE HWSPEC_VERSION="${PROJECT_VERSION}")
if(WIN32)
  target_link_libraries(hwspec_core PRIVATE advapi32 user32 dxgi ole32 oleaut32 wbemuuid iphlpapi ws2_32 setupapi powrprof)
endif()
hwspec_warnings(hwspec_core)

# ---- CLI library: argument parsing and run(), shared by the .exe and the tests ----
add_library(hwspec_cli STATIC cli/args.cpp cli/admin.cpp cli/run.cpp)
target_include_directories(hwspec_cli PUBLIC cli)
target_link_libraries(hwspec_cli PUBLIC hwspec_core)
if(WIN32)
  target_link_libraries(hwspec_cli PRIVATE shell32 advapi32)
endif()
hwspec_warnings(hwspec_cli)

# ---- The program: a thin main() over the CLI library ----
add_executable(hwspec cli/main.cpp)
target_link_libraries(hwspec PRIVATE hwspec_cli)
hwspec_warnings(hwspec)

# ---- Tests ----
if(HWSPEC_BUILD_TESTS)
  enable_testing()

  function(hwspec_add_test name)
    add_executable(${name} tests/${name}.cpp)
    target_link_libraries(${name} PRIVATE hwspec_cli)
    hwspec_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
  endfunction()

  hwspec_add_test(test_version)
  target_compile_definitions(test_version PRIVATE EXPECTED_VERSION="${PROJECT_VERSION}")

  hwspec_add_test(test_format)
  hwspec_add_test(test_args)
  hwspec_add_test(test_run)
  hwspec_add_test(test_m2)
  hwspec_add_test(test_admin)

  if(WIN32)
    hwspec_add_test(test_collectors)  # runs the real collectors, so Windows only
  endif()
endif()
