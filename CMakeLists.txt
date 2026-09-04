cmake_minimum_required(VERSION 3.16)
project(testpulse_gtest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TESTPULSE_GTEST_BUILD_TESTS "Build testpulse-gtest's own test suite" ON)
option(TESTPULSE_GTEST_BUILD_CLI "Build the testpulse-gtest submit CLI" ON)

find_package(Threads REQUIRED)

add_library(testpulse INTERFACE)
add_library(testpulse::testpulse ALIAS testpulse)
target_include_directories(testpulse INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(testpulse INTERFACE cxx_std_17)

if(TESTPULSE_GTEST_BUILD_TESTS)
    enable_testing()

    # Fetched once here (not inside tests/ or cli/tests/) so both
    # subdirectories can link against it regardless of add_subdirectory
    # order -- pinned to a released tag, never a branch, per design.md's
    # tag-pinning decision (the same discipline this design requires of
    # consumers' own FetchContent_Declare calls for this library).
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.18.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

if(TESTPULSE_GTEST_BUILD_CLI)
    add_subdirectory(cli)
endif()

if(TESTPULSE_GTEST_BUILD_TESTS)
    add_subdirectory(tests)
endif()
