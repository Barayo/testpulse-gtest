#include <gtest/gtest.h>

#include "stub_server.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using testpulse_cli_test::StubServer;

namespace {

const char* kSimpleReport = R"(<testsuites><testsuite name="X"><testcase name="A">
  <properties><property name="testpulse_case_key" value="LOGIN-42"/></properties>
</testcase></testsuite></testsuites>)";

fs::path WriteReport(const fs::path& dir) {
    fs::create_directories(dir);
    fs::path path = dir / "report.xml";
    std::ofstream file(path);
    file << kSimpleReport;
    return path;
}

int RunCli(const std::string& args) {
    std::string command = std::string(TESTPULSE_GTEST_CLI_PATH) + " " + args + " > /tmp/testpulse_cli_stdout.txt 2>&1";
    int status = std::system(command.c_str());
    return WEXITSTATUS(status);
}

std::string LastOutput() {
    std::ifstream file("/tmp/testpulse_cli_stdout.txt");
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

}  // namespace

TEST(SubmitE2ETest, RealSubmissionAgainstAStubServerSucceeds) {
    StubServer server([](const std::string& method, const std::string& path, const std::string&) {
        EXPECT_EQ(method, "POST");
        EXPECT_EQ(path, "/api/v1/projects/LOGIN/imports");
        return std::make_pair(201, std::string(R"({"id":"r1","key":"LOGIN-R1"})"));
    });

    fs::path workDir = fs::temp_directory_path() / "testpulse_cli_e2e_submit";
    fs::path reportPath = WriteReport(workDir);

    std::string command = "--file " + reportPath.string() + " --url " + server.Url() +
                           " --project LOGIN --token t0k3n";
    int code = RunCli("submit " + command);

    EXPECT_EQ(code, 0);
    EXPECT_NE(LastOutput().find("LOGIN-R1"), std::string::npos) << LastOutput();
}

TEST(SubmitE2ETest, RealDryRunAgainstAStubServerPreviewsWithoutSubmitting) {
    bool postCalled = false;
    StubServer server([&postCalled](const std::string& method, const std::string& path,
                                     const std::string&) -> std::pair<int, std::string> {
        if (method == "POST") {
            postCalled = true;
            return {201, "{}"};
        }
        EXPECT_EQ(path, "/api/v1/projects/LOGIN/cases");
        return {200, R"([{"key":"LOGIN-42"}])"};
    });

    fs::path workDir = fs::temp_directory_path() / "testpulse_cli_e2e_dryrun";
    fs::path reportPath = WriteReport(workDir);

    std::string command = "--file " + reportPath.string() + " --url " + server.Url() +
                           " --project LOGIN --token t0k3n --dry-run";
    int code = RunCli("submit " + command);

    EXPECT_EQ(code, 0);
    EXPECT_FALSE(postCalled);
    EXPECT_NE(LastOutput().find("would match: LOGIN-42"), std::string::npos) << LastOutput();
}
