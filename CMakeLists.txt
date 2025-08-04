cmake_minimum_required(VERSION 3.15)
project(rsl)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 26)

add_library(kwargs INTERFACE)
target_include_directories(kwargs INTERFACE include)

target_compile_options(kwargs INTERFACE "-stdlib=libc++" "-freflection")
target_compile_options(kwargs INTERFACE "-Wno-c++26-extensions")

target_link_options(kwargs INTERFACE "-stdlib=libc++" "-lc++abi")

option(BUILD_TESTING "Enable tests" ON)
option(BUILD_EXAMPLES "Enable examples" ON)
option(ENABLE_COVERAGE "Enable coverage instrumentation" OFF)
option(ENABLE_FORMAT "Enable formatting extension" ON)

if (ENABLE_FORMAT)
  target_compile_definitions(kwargs INTERFACE KWARGS_FORMAT=1)
endif()

if (BUILD_TESTING)
  message(STATUS "Building unit tests")

  enable_testing()
  add_executable(kwargs_tests "")
  add_subdirectory(tests)

  find_package(GTest REQUIRED)
  target_link_libraries(kwargs_tests PRIVATE kwargs)
  target_link_libraries(kwargs_tests PRIVATE GTest::gtest GTest::gmock)

  include(GoogleTest)
  gtest_discover_tests(kwargs_tests)


  if(ENABLE_COVERAGE)
    message(STATUS "Instrumenting for coverage")  

    target_compile_options(kwargs_tests PRIVATE -g -O0 --coverage)
    target_link_libraries(kwargs_tests PRIVATE --coverage)
  endif()
endif()

if (BUILD_EXAMPLES)
  add_subdirectory(example)
endif()