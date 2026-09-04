#include <testpulse_cli/config.hpp>

#include <stdexcept>

namespace testpulse_cli {

namespace {

std::string ResolveRequired(const std::optional<std::string>& flag, const GetEnvFn& getEnv,
                             const char* envName, const char* settingName) {
    if (flag) {
        return *flag;
    }
    const char* fromEnv = getEnv(envName);
    if (fromEnv != nullptr) {
        return std::string(fromEnv);
    }
    throw std::invalid_argument(std::string("testpulse: ") + settingName +
                                 " is required (set the corresponding flag, or " + envName + ")");
}

bool ParseBool(const std::string& value) { return value == "true" || value == "1"; }

}  // namespace

Config ResolveConfig(const CliFlags& flags, const GetEnvFn& getEnv) {
    Config config;
    config.url = ResolveRequired(flags.url, getEnv, "TESTPULSE_URL", "--url");
    config.token = ResolveRequired(flags.token, getEnv, "TESTPULSE_TOKEN", "--token");
    config.project = ResolveRequired(flags.project, getEnv, "TESTPULSE_PROJECT", "--project");

    if (flags.failOnUnmatched) {
        config.failOnUnmatched = *flags.failOnUnmatched;
    } else if (const char* fromEnv = getEnv("TESTPULSE_FAIL_ON_UNMATCHED"); fromEnv != nullptr) {
        config.failOnUnmatched = ParseBool(fromEnv);
    }

    config.dryRun = flags.dryRun;
    config.dir = flags.dir;
    return config;
}

std::string Redacted(const Config& config) {
    return "Config{url=" + config.url + ", token=(redacted), project=" + config.project +
           ", failOnUnmatched=" + (config.failOnUnmatched ? "true" : "false") +
           ", dryRun=" + (config.dryRun ? "true" : "false") + ", dir=" + config.dir + "}";
}

}  // namespace testpulse_cli
