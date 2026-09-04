cmake_minimum_required(VERSION 3.16)
project(testpulse_gtest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TESTPULSE_GTEST_BUILD_TESTS "Build testpulse-gtest's own test suite" ON)
option(TESTPULSE_GTEST_BUILD_CLI "Build the testpulse-gtest submit CLI" ON)

add_library(testpulse INTERFACE)
add_library(testpulse::testpulse ALIAS testpulse)
target_include_directories(testpulse INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(testpulse INTERFACE cxx_std_17)

if(TESTPULSE_GTEST_BUILD_CLI)
    add_subdirectory(cli)
endif()

if(TESTPULSE_GTEST_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
