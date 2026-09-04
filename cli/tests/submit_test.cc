#include <gtest/gtest.h>
#include <testpulse_cli/submit.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

namespace fs = std::filesystem;
using testpulse_cli::Config;
using testpulse_cli::HttpClient;
using testpulse_cli::HttpResponse;
using testpulse_cli::RunSubmit;

namespace {

class FakeHttpClient : public HttpClient {
public:
    std::function<HttpResponse(const std::string&, const std::string&, const std::string&)> onPost;
    std::function<HttpResponse(const std::string&, const std::string&)> onGet;
    std::string lastPostUrl;
    std::string lastPostBody;
    std::string lastGetUrl;

    HttpResponse Post(const std::string& url, const std::string& body,
                       const std::string& bearerToken) override {
        lastPostUrl = url;
        lastPostBody = body;
        return onPost(url, body, bearerToken);
    }

    HttpResponse Get(const std::string& url, const std::string& bearerToken) override {
        lastGetUrl = url;
        return onGet(url, bearerToken);
    }
};

Config MinimalConfig() {
    Config config;
    config.url = "http://testpulse.example";
    config.token = "t0k3n";
    config.project = "LOGIN";
    return config;
}

std::string WriteTempReport(const std::string& xml) {
    fs::path path = fs::temp_directory_path() / "testpulse_cli_submit_report.xml";
    std::ofstream file(path);
    file << xml;
    return path.string();
}

const char* kSimpleReport = R"(<testsuites><testsuite name="X"><testcase name="A">
  <properties><property name="testpulse_case_key" value="LOGIN-42"/></properties>
</testcase></testsuite></testsuites>)";

}  // namespace

TEST(SubmitTest, AllMatchedExitsZero) {
    FakeHttpClient client;
    client.onPost = [](const std::string&, const std::string&, const std::string&) {
        return HttpResponse{201, R"({"id":"r1","key":"LOGIN-R1"})", false, ""};
    };
    std::ostringstream out, err;
    int code = RunSubmit(MinimalConfig(), WriteTempReport(kSimpleReport), client, out, err);
    EXPECT_EQ(code, 0);
    EXPECT_NE(out.str().find("LOGIN-R1"), std::string::npos);
}

TEST(SubmitTest, UnmatchedDefaultConfigExitsZero) {
    FakeHttpClient client;
    client.onPost = [](const std::string&, const std::string&, const std::string&) {
        return HttpResponse{
            207,
            R"({"run":{"id":"r1","key":"LOGIN-R2"},"message":"1 unmatched","matched":0,"unmatched":[{"caseKey":"LOGIN-42","verdict":"passed"}]})",
            false, ""};
    };
    std::ostringstream out, err;
    Config config = MinimalConfig();
    int code = RunSubmit(config, WriteTempReport(kSimpleReport), client, out, err);
    EXPECT_EQ(code, 0);
    EXPECT_NE(out.str().find("LOGIN-42"), std::string::npos);
    EXPECT_NE(out.str().find("fail-on-unmatched"), std::string::npos);
}

TEST(SubmitTest, UnmatchedWithFailOnUnmatchedExitsNonZero) {
    FakeHttpClient client;
    client.onPost = [](const std::string&, const std::string&, const std::string&) {
        return HttpResponse{
            207,
            R"({"run":{"id":"r1","key":"LOGIN-R3"},"message":"1 unmatched","matched":0,"unmatched":[{"caseKey":"LOGIN-42","verdict":"passed"}]})",
            false, ""};
    };
    std::ostringstream out, err;
    Config config = MinimalConfig();
    config.failOnUnmatched = true;
    int code = RunSubmit(config, WriteTempReport(kSimpleReport), client, out, err);
    EXPECT_NE(code, 0);
}

TEST(SubmitTest, NetworkErrorExitsNonZeroRegardlessOfFailOnUnmatched) {
    FakeHttpClient client;
    client.onPost = [](const std::string&, const std::string&, const std::string&) {
        return HttpResponse{0, "", true, "connection refused"};
    };
    std::ostringstream out, err;
    int code = RunSubmit(MinimalConfig(), WriteTempReport(kSimpleReport), client, out, err);
    EXPECT_NE(code, 0);
}

TEST(SubmitTest, ServerErrorExitsNonZero) {
    FakeHttpClient client;
    client.onPost = [](const std::string&, const std::string&, const std::string&) {
        return HttpResponse{500, R"({"error":"boom"})", false, ""};
    };
    std::ostringstream out, err;
    int code = RunSubmit(MinimalConfig(), WriteTempReport(kSimpleReport), client, out, err);
    EXPECT_NE(code, 0);
}

TEST(SubmitTest, DryRunPreviewsWithoutSubmitting) {
    FakeHttpClient client;
    bool postCalled = false;
    client.onPost = [&postCalled](const std::string&, const std::string&, const std::string&) {
        postCalled = true;
        return HttpResponse{201, "{}", false, ""};
    };
    client.onGet = [](const std::string&, const std::string&) {
        return HttpResponse{200, R"([{"key":"LOGIN-42"},{"key":"LOGIN-99"}])", false, ""};
    };
    std::ostringstream out, err;
    Config config = MinimalConfig();
    config.dryRun = true;
    int code = RunSubmit(config, WriteTempReport(kSimpleReport), client, out, err);
    EXPECT_EQ(code, 0);
    EXPECT_FALSE(postCalled);
    EXPECT_NE(out.str().find("LOGIN-42"), std::string::npos);
}

TEST(SubmitTest, DryRunFetchFailureExitsNonZero) {
    FakeHttpClient client;
    client.onGet = [](const std::string&, const std::string&) {
        return HttpResponse{0, "", true, "connection refused"};
    };
    std::ostringstream out, err;
    Config config = MinimalConfig();
    config.dryRun = true;
    int code = RunSubmit(config, WriteTempReport(kSimpleReport), client, out, err);
    EXPECT_NE(code, 0);
}

// A shared CI workspace can have stale .testpulse/attachments left over
// from an unrelated earlier test binary/run -- ReadAttachments has no
// awareness of which report is currently being submitted, so submit()
// itself must filter to only the case keys the CURRENT report actually
// declares, or a stale attachment silently rides along into an unrelated
// submission (confirmed as a real bug: a live server correctly rejected
// a submission this way with "attachment is not a supported image
// format" for a report that never called Attach() at all).
TEST(SubmitTest, OnlyAttachmentsForCaseKeysInThisReportAreSubmitted) {
    fs::path dir = fs::temp_directory_path() / "testpulse_cli_submit_stale_attach";
    fs::remove_all(dir);
    fs::path attachDir = dir / ".testpulse" / "attachments";
    fs::create_directories(attachDir);
    {
        std::ofstream dataFile(attachDir / "stale.data", std::ios::binary);
        dataFile << "x";
        std::ofstream metaFile(attachDir / "stale.json");
        metaFile << R"({"caseKey":"OTHER-99","filename":"a.png","contentType":"image/png"})";
    }

    FakeHttpClient client;
    client.onPost = [](const std::string&, const std::string&, const std::string&) {
        return HttpResponse{201, R"({"id":"r1","key":"LOGIN-R1"})", false, ""};
    };
    std::ostringstream out, err;
    Config config = MinimalConfig();
    config.dir = dir.string();
    RunSubmit(config, WriteTempReport(kSimpleReport), client, out, err);

    nlohmann::json body = nlohmann::json::parse(client.lastPostBody);
    EXPECT_TRUE(body.at("attachments").empty())
        << "stale OTHER-99 attachment should have been excluded: " << client.lastPostBody;
}

TEST(SubmitTest, TokenNeverAppearsInOutput) {
    FakeHttpClient client;
    client.onPost = [](const std::string&, const std::string&, const std::string&) {
        return HttpResponse{500, R"({"error":"boom"})", false, ""};
    };
    std::ostringstream out, err;
    int code = RunSubmit(MinimalConfig(), WriteTempReport(kSimpleReport), client, out, err);
    (void)code;
    EXPECT_EQ(out.str().find("t0k3n"), std::string::npos);
    EXPECT_EQ(err.str().find("t0k3n"), std::string::npos);
}
