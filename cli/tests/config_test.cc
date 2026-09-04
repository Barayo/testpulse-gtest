#include <gtest/gtest.h>
#include <testpulse_cli/config.hpp>

#include <map>
#include <stdexcept>
#include <string>

using testpulse_cli::CliFlags;
using testpulse_cli::Config;
using testpulse_cli::ResolveConfig;

namespace {

testpulse_cli::GetEnvFn EnvFrom(std::map<std::string, std::string> vars) {
    return [vars](const char* name) -> const char* {
        auto it = vars.find(name);
        if (it == vars.end()) {
            return nullptr;
        }
        return it->second.c_str();
    };
}

CliFlags MinimalFlags() {
    CliFlags flags;
    flags.url = "http://a.example";
    flags.token = "t0k3n";
    flags.project = "LOGIN";
    return flags;
}

}  // namespace

TEST(ConfigTest, UrlFlagAlone) {
    Config config = ResolveConfig(MinimalFlags(), EnvFrom({}));
    EXPECT_EQ(config.url, "http://a.example");
}

TEST(ConfigTest, UrlEnvVarAlone) {
    CliFlags flags;
    flags.token = "t0k3n";
    flags.project = "LOGIN";
    Config config = ResolveConfig(flags, EnvFrom({{"TESTPULSE_URL", "http://b.example"}}));
    EXPECT_EQ(config.url, "http://b.example");
}

TEST(ConfigTest, UrlFlagOverridesEnvVar) {
    CliFlags flags = MinimalFlags();
    Config config = ResolveConfig(flags, EnvFrom({{"TESTPULSE_URL", "http://b.example"}}));
    EXPECT_EQ(config.url, "http://a.example");
}

TEST(ConfigTest, ProjectFlagOverridesEnvVar) {
    CliFlags flags = MinimalFlags();
    Config config = ResolveConfig(flags, EnvFrom({{"TESTPULSE_PROJECT", "OTHER"}}));
    EXPECT_EQ(config.project, "LOGIN");
}

TEST(ConfigTest, FailOnUnmatchedFlagOverridesEnvVar) {
    CliFlags flags = MinimalFlags();
    flags.failOnUnmatched = true;
    Config config = ResolveConfig(flags, EnvFrom({{"TESTPULSE_FAIL_ON_UNMATCHED", "false"}}));
    EXPECT_TRUE(config.failOnUnmatched);
}

TEST(ConfigTest, FailOnUnmatchedEnvVarAlone) {
    CliFlags flags = MinimalFlags();
    Config config = ResolveConfig(flags, EnvFrom({{"TESTPULSE_FAIL_ON_UNMATCHED", "true"}}));
    EXPECT_TRUE(config.failOnUnmatched);
}

TEST(ConfigTest, TokenEnvVarAlone) {
    CliFlags flags;
    flags.url = "http://a.example";
    flags.project = "LOGIN";
    Config config = ResolveConfig(flags, EnvFrom({{"TESTPULSE_TOKEN", "env-token"}}));
    EXPECT_EQ(config.token, "env-token");
}

TEST(ConfigTest, TokenFlagOverridesEnvVar) {
    CliFlags flags = MinimalFlags();
    Config config = ResolveConfig(flags, EnvFrom({{"TESTPULSE_TOKEN", "env-token"}}));
    EXPECT_EQ(config.token, "t0k3n");
}

TEST(ConfigTest, MissingSettingErrorMessageHasNoDoubledPrefix) {
    // main.cc's catch site prints "testpulse: " + e.what() -- the
    // exception message itself must not also carry that prefix, or the
    // real CLI prints "testpulse: testpulse: --url is required".
    CliFlags flags;
    flags.token = "t0k3n";
    flags.project = "LOGIN";
    try {
        ResolveConfig(flags, EnvFrom({}));
        FAIL() << "expected std::invalid_argument";
    } catch (const std::invalid_argument& e) {
        EXPECT_EQ(std::string(e.what()).find("testpulse:"), std::string::npos) << e.what();
    }
}

TEST(ConfigTest, MissingUrlThrows) {
    CliFlags flags;
    flags.token = "t0k3n";
    flags.project = "LOGIN";
    EXPECT_THROW(ResolveConfig(flags, EnvFrom({})), std::invalid_argument);
}

TEST(ConfigTest, MissingTokenThrows) {
    CliFlags flags;
    flags.url = "http://a.example";
    flags.project = "LOGIN";
    EXPECT_THROW(ResolveConfig(flags, EnvFrom({})), std::invalid_argument);
}

TEST(ConfigTest, MissingProjectThrows) {
    CliFlags flags;
    flags.url = "http://a.example";
    flags.token = "t0k3n";
    EXPECT_THROW(ResolveConfig(flags, EnvFrom({})), std::invalid_argument);
}

TEST(ConfigTest, RedactedNeverContainsTheRawToken) {
    Config config = ResolveConfig(MinimalFlags(), EnvFrom({}));
    std::string redacted = testpulse_cli::Redacted(config);
    EXPECT_EQ(redacted.find("t0k3n"), std::string::npos);
}
