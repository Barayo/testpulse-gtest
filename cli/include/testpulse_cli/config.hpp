#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace testpulse_cli {

// Resolved from, in order per setting: the CLI flag, then the
// environment variable. No third, config-file-backed tier -- C++ has no
// pom.xml/build.gradle/.csproj equivalent to hang config off of.
struct Config {
    std::string url;
    std::string token;
    std::string project;
    bool failOnUnmatched = false;
    bool dryRun = false;
    std::string dir = ".";
};

struct CliFlags {
    std::optional<std::string> url;
    std::optional<std::string> token;
    std::optional<std::string> project;
    std::optional<bool> failOnUnmatched;
    bool dryRun = false;
    std::string dir = ".";
};

using GetEnvFn = std::function<const char*(const char*)>;

// Throws std::invalid_argument naming the missing setting if url/token/
// project can't be resolved from either source.
Config ResolveConfig(const CliFlags& flags, const GetEnvFn& getEnv);

// Never includes the resolved token -- printed/logged wherever a Config
// needs to appear in output.
std::string Redacted(const Config& config);

}  // namespace testpulse_cli
