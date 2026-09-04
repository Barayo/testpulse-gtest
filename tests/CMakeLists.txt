add_executable(testpulse_gtest_tests
    case_test.cc
    attach_test.cc
)
target_link_libraries(testpulse_gtest_tests PRIVATE testpulse::testpulse gtest gtest_main)
target_compile_features(testpulse_gtest_tests PRIVATE cxx_std_17)

add_subdirectory(fixtures)

add_executable(testpulse_gtest_e2e_tests e2e_test.cc)
target_link_libraries(testpulse_gtest_e2e_tests PRIVATE gtest gtest_main)
target_compile_features(testpulse_gtest_e2e_tests PRIVATE cxx_std_17)
target_compile_definitions(testpulse_gtest_e2e_tests PRIVATE
    "TAGGED_FIXTURE_PATH=\"$<TARGET_FILE:tagged_fixture>\""
    "CROSS_TEST_FIXTURE_PATH=\"$<TARGET_FILE:cross_test_fixture>\""
    "MULTI_ATTACH_FIXTURE_PATH=\"$<TARGET_FILE:multi_attach_fixture>\""
)
add_dependencies(testpulse_gtest_e2e_tests tagged_fixture cross_test_fixture multi_attach_fixture)

include(GoogleTest)
gtest_discover_tests(testpulse_gtest_tests)
gtest_discover_tests(testpulse_gtest_e2e_tests)
