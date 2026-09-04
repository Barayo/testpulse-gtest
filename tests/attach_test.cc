#include <gtest/gtest.h>
#include <testpulse/testpulse.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::vector<uint8_t> SampleBytes() { return {0x01, 0x02, 0x03}; }

int CountFilesWithSuffix(const fs::path& dir, const std::string& suffix) {
    if (!fs::exists(dir)) {
        return 0;
    }
    int count = 0;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().string().size() >= suffix.size() &&
            entry.path().string().compare(entry.path().string().size() - suffix.size(), suffix.size(),
                                           suffix) == 0) {
            ++count;
        }
    }
    return count;
}

}  // namespace

TEST(AttachTest, SucceedsForDeclaredCaseKey) {
    testpulse::Case("LOGIN-42");
    EXPECT_NO_THROW(testpulse::Attach("LOGIN-42", SampleBytes(), "failure.png", "image/png"));
}

TEST(AttachTest, RejectsUndeclaredCaseKey) {
    testpulse::Case("LOGIN-43");
    EXPECT_THROW(testpulse::Attach("OTHER-1", SampleBytes(), "failure.png", "image/png"),
                 testpulse::TestPulseError);
}

TEST(AttachTest, RejectsUnsupportedContentTypeEvenForDeclaredCaseKey) {
    testpulse::Case("LOGIN-44");
    EXPECT_THROW(testpulse::Attach("LOGIN-44", SampleBytes(), "x.pdf", "application/pdf"),
                 testpulse::TestPulseError);
}

TEST(AttachTest, ContentTypeIsValidatedBeforeAllowlistCheck) {
    // No Case() call at all -- if content-type validation ran after the
    // allowlist check, this would fail with the allowlist error instead.
    try {
        testpulse::Attach("NEVER-DECLARED", SampleBytes(), "x.pdf", "application/pdf");
        FAIL() << "expected TestPulseError";
    } catch (const testpulse::TestPulseError& e) {
        EXPECT_NE(std::string(e.what()).find("content type"), std::string::npos);
    }
}

TEST(AttachTest, TwoAttachmentsUnderSameCaseKeyBothSurvive) {
    testpulse::Case("LOGIN-45");
    fs::path dir = fs::current_path() / ".testpulse" / "attachments";
    if (fs::exists(dir)) {
        fs::remove_all(dir);
    }
    testpulse::Attach("LOGIN-45", {0x01}, "a.png", "image/png");
    testpulse::Attach("LOGIN-45", {0x02}, "b.png", "image/png");
    EXPECT_EQ(CountFilesWithSuffix(dir, ".data"), 2);
    EXPECT_EQ(CountFilesWithSuffix(dir, ".json"), 2);
}

// A real proof that Attach() requires an active test, not a simulated
// one: gtest's ::testing::Environment::SetUp() runs once before any
// TEST/TEST_F executes, when current_test_info() is genuinely null (this
// is a real, documented gtest extensibility hook, not a workaround) --
// calling Attach() from there exercises the actual null-current_test_info()
// path, with the result captured for a later TEST to assert on (throwing
// out of Environment::SetUp() itself would abort the whole binary).
namespace {

bool g_outsideTestThrew = false;
std::string g_outsideTestErrorMessage;

class OutsideTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        try {
            testpulse::Attach("LOGIN-46", SampleBytes(), "failure.png", "image/png");
        } catch (const testpulse::TestPulseError& e) {
            g_outsideTestThrew = true;
            g_outsideTestErrorMessage = e.what();
        }
    }
};

::testing::Environment* const outside_test_env =
    ::testing::AddGlobalTestEnvironment(new OutsideTestEnvironment);

}  // namespace

TEST(AttachTest, OutsideActiveTestThrowsDistinctError) {
    EXPECT_TRUE(g_outsideTestThrew);
    EXPECT_NE(g_outsideTestErrorMessage.find("outside an active test"), std::string::npos)
        << "actual message: " << g_outsideTestErrorMessage;
}
