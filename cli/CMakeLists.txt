# testpulse-gtest submit CLI. Depends on nlohmann/json (JSON request/
# response bodies) and libcurl (HTTP) -- C++ has no stdlib equivalent of
# either, unlike every other language this plugin family targets.
# nlohmann/json is fetched and pinned like gtest itself; libcurl is a
# system dependency resolved via find_package, present by default on
# Linux/macOS and installed explicitly in CI on Windows -- prebuilt
# release binaries bundle it so end users installing a release need
# nothing extra.

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

find_package(CURL REQUIRED)

add_library(testpulse_cli_lib
    src/config.cc
    src/report_reader.cc
    src/attachments.cc
    src/http_client.cc
    src/submit.cc
)
target_include_directories(testpulse_cli_lib PUBLIC include)
target_link_libraries(testpulse_cli_lib PUBLIC nlohmann_json::nlohmann_json CURL::libcurl)
target_compile_features(testpulse_cli_lib PUBLIC cxx_std_17)

add_executable(testpulse-gtest src/main.cc)
target_link_libraries(testpulse-gtest PRIVATE testpulse_cli_lib)

if(TESTPULSE_GTEST_BUILD_TESTS)
    add_subdirectory(tests)
endif()
