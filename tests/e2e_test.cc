#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

std::string ReadFile(const fs::path& path) {
    std::ifstream file(path);
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

// Runs a fixture binary in a fresh temporary working directory (so its
// .testpulse/ scratch dir and gtest_output=xml file don't collide across
// tests), returns the exit code.
int RunFixture(const std::string& fixturePath, const fs::path& workDir, const fs::path& xmlOut) {
    fs::create_directories(workDir);
    std::string command = "cd " + workDir.string() + " && " + fixturePath +
                           " --gtest_output=xml:" + xmlOut.string() + " > /dev/null 2>&1";
    return std::system(command.c_str());
}

}  // namespace

TEST(E2ETaggedFixture, RealGeneratedXmlContainsCaseKeyProperty) {
    fs::path workDir = fs::temp_directory_path() / "testpulse_gtest_e2e_tagged";
    if (fs::exists(workDir)) {
        fs::remove_all(workDir);
    }
    fs::path xmlOut = workDir / "report.xml";

    int code = RunFixture(TAGGED_FIXTURE_PATH, workDir, xmlOut);
    ASSERT_EQ(code, 0) << "fixture binary itself failed";

    std::string xml = ReadFile(xmlOut);
    ASSERT_FALSE(xml.empty()) << "no XML report was produced";
    EXPECT_NE(xml.find(R"(<property name="testpulse_case_key" value="LOGIN-42"/>)"), std::string::npos)
        << "actual XML:\n" << xml;

    // The untagged test must carry no testpulse_case_key property at all.
    // A crude but adequate check for this fixture's known, simple shape:
    // find the <testcase> for UntaggedTest and confirm no <properties>
    // block follows before its closing tag.
    auto untaggedPos = xml.find(R"(name="UntaggedTest")");
    ASSERT_NE(untaggedPos, std::string::npos);
    auto closePos = xml.find("</testcase>", untaggedPos);
    auto selfClosePos = xml.find("/>", untaggedPos);
    bool selfClosed = selfClosePos != std::string::npos &&
                       (closePos == std::string::npos || selfClosePos < closePos);
    if (!selfClosed) {
        std::string segment = xml.substr(untaggedPos, closePos - untaggedPos);
        EXPECT_EQ(segment.find("testpulse_case_key"), std::string::npos) << "actual XML:\n" << xml;
    }
}

TEST(E2ECrossTestFixture, AttachUnderAnotherTestsCaseKeyIsRejected) {
    fs::path workDir = fs::temp_directory_path() / "testpulse_gtest_e2e_cross_test";
    if (fs::exists(workDir)) {
        fs::remove_all(workDir);
    }
    fs::path xmlOut = workDir / "report.xml";

    int code = RunFixture(CROSS_TEST_FIXTURE_PATH, workDir, xmlOut);
    // Both TEST cases in cross_test_fixture pass (the second one's own
    // assertion is that Attach() threw, caught internally as SUCCEED()) --
    // the real content of this proof is inside the fixture's own XML/
    // exit code, not this outer process's exit code alone.
    EXPECT_EQ(code, 0) << "cross_test_fixture itself reported a failure -- "
                           "either the case-key rejection didn't happen, "
                           "or something else broke";
}

TEST(E2EMultiAttachFixture, BothAttachmentsSurviveOnDisk) {
    fs::path workDir = fs::temp_directory_path() / "testpulse_gtest_e2e_multi_attach";
    if (fs::exists(workDir)) {
        fs::remove_all(workDir);
    }
    fs::path xmlOut = workDir / "report.xml";

    int code = RunFixture(MULTI_ATTACH_FIXTURE_PATH, workDir, xmlOut);
    ASSERT_EQ(code, 0);

    fs::path attachDir = workDir / ".testpulse" / "attachments";
    ASSERT_TRUE(fs::exists(attachDir)) << "no .testpulse/attachments directory was created";

    int dataFiles = 0;
    for (const auto& entry : fs::directory_iterator(attachDir)) {
        if (entry.path().extension() == ".data") {
            ++dataFiles;
        }
    }
    EXPECT_EQ(dataFiles, 2);
}
