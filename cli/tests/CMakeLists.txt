add_executable(testpulse_cli_tests
    config_test.cc
    report_reader_test.cc
    attachments_test.cc
    submit_test.cc
)
target_link_libraries(testpulse_cli_tests PRIVATE testpulse_cli_lib gtest gtest_main)
target_compile_features(testpulse_cli_tests PRIVATE cxx_std_17)

add_executable(testpulse_cli_e2e_tests submit_e2e_test.cc)
target_link_libraries(testpulse_cli_e2e_tests PRIVATE gtest gtest_main Threads::Threads)
target_compile_features(testpulse_cli_e2e_tests PRIVATE cxx_std_17)
target_compile_definitions(testpulse_cli_e2e_tests PRIVATE
    "TESTPULSE_GTEST_CLI_PATH=\"$<TARGET_FILE:testpulse-gtest>\""
)
add_dependencies(testpulse_cli_e2e_tests testpulse-gtest)

include(GoogleTest)
gtest_discover_tests(testpulse_cli_tests)
gtest_discover_tests(testpulse_cli_e2e_tests)
