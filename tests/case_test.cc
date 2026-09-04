#include <gtest/gtest.h>
#include <testpulse/testpulse.hpp>

#include <string>
#include <vector>

// These are unit tests of Case()'s effect on RecordProperty, verified by
// inspecting the TestResult gtest itself records for the currently-running
// test via current_test_info()->result() -- this lets us assert on
// recorded properties without a full XML round-trip (that's what the
// real end-to-end tests, section 2.4/2.5's fixture binaries, are for).

namespace {

std::string PropertyValue(const std::string& key) {
    const ::testing::TestResult* result =
        ::testing::UnitTest::GetInstance()->current_test_info()->result();
    for (int i = 0; i < result->test_property_count(); ++i) {
        const ::testing::TestProperty& prop = result->GetTestProperty(i);
        if (key == prop.key()) {
            return prop.value();
        }
    }
    return {};
}

bool HasProperty(const std::string& key) {
    const ::testing::TestResult* result =
        ::testing::UnitTest::GetInstance()->current_test_info()->result();
    for (int i = 0; i < result->test_property_count(); ++i) {
        if (key == result->GetTestProperty(i).key()) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST(CaseTest, RecordsCaseKeyProperty) {
    testpulse::Case("LOGIN-42");
    EXPECT_EQ(PropertyValue("testpulse_case_key"), "LOGIN-42");
}

TEST(CaseTest, RecordsPlatformWhenSupplied) {
    testpulse::Case("LOGIN-42", testpulse::WithPlatform("linux"));
    EXPECT_EQ(PropertyValue("testpulse_platform"), "linux");
}

TEST(CaseTest, DoesNotRecordPlatformWhenNotSupplied) {
    testpulse::Case("LOGIN-42");
    EXPECT_FALSE(HasProperty("testpulse_platform"));
}

TEST(CaseTest, RecordsVersionWhenSupplied) {
    testpulse::Case("LOGIN-42", testpulse::WithVersion("2.0"));
    EXPECT_EQ(PropertyValue("testpulse_version"), "2.0");
}

TEST(CaseTest, DoesNotRecordVersionWhenNotSupplied) {
    testpulse::Case("LOGIN-42");
    EXPECT_FALSE(HasProperty("testpulse_version"));
}

TEST(CaseTest, RecordsTagsWhenSupplied) {
    testpulse::Case("LOGIN-42", testpulse::WithTags({"smoke", "auth"}));
    EXPECT_EQ(PropertyValue("testpulse_tags"), "smoke,auth");
}

TEST(CaseTest, DoesNotRecordTagsWhenNotSupplied) {
    testpulse::Case("LOGIN-42");
    EXPECT_FALSE(HasProperty("testpulse_tags"));
}

TEST(CaseTest, RecordsAllOptionsTogether) {
    testpulse::Case("LOGIN-42", testpulse::WithPlatform("linux"),
                     testpulse::WithVersion("2.0"), testpulse::WithTags({"smoke"}));
    EXPECT_EQ(PropertyValue("testpulse_platform"), "linux");
    EXPECT_EQ(PropertyValue("testpulse_version"), "2.0");
    EXPECT_EQ(PropertyValue("testpulse_tags"), "smoke");
}
