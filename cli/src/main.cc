#include <testpulse_cli/config.hpp>
#include <testpulse_cli/http_client.hpp>
#include <testpulse_cli/submit.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using testpulse_cli::CliFlags;
using testpulse_cli::Config;
using testpulse_cli::CurlHttpClient;
using testpulse_cli::ResolveConfig;
using testpulse_cli::RunSubmit;

namespace {

const char* GetEnvOrNull(const char* name) { return std::getenv(name); }

// Accepts both "--flag value" and "--flag=value" forms.
std::optional<std::string> TakeValue(const std::vector<std::string>& args, size_t& i,
                                      const std::string& flag) {
    const std::string& arg = args[i];
    if (arg == flag && i + 1 < args.size()) {
        return args[++i];
    }
    if (arg.rfind(flag + "=", 0) == 0) {
        return arg.substr(flag.size() + 1);
    }
    return std::nullopt;
}

}  // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);

    if (args.empty() || args[0] != "submit") {
        std::cerr << "usage: testpulse-gtest submit --file <report.xml> [--url URL] "
                     "[--project KEY] [--token TOKEN] [--fail-on-unmatched] [--dry-run] "
                     "[--dir DIR]\n";
        return 2;
    }

    std::string reportFile;
    CliFlags flags;

    for (size_t i = 1; i < args.size(); ++i) {
        if (auto v = TakeValue(args, i, "--file")) {
            reportFile = *v;
        } else if (auto v = TakeValue(args, i, "--url")) {
            flags.url = *v;
        } else if (auto v = TakeValue(args, i, "--token")) {
            flags.token = *v;
        } else if (auto v = TakeValue(args, i, "--project")) {
            flags.project = *v;
        } else if (auto v = TakeValue(args, i, "--dir")) {
            flags.dir = *v;
        } else if (args[i] == "--fail-on-unmatched") {
            flags.failOnUnmatched = true;
        } else if (args[i] == "--dry-run") {
            flags.dryRun = true;
        } else {
            std::cerr << "testpulse: unrecognized argument '" << args[i] << "'\n";
            return 2;
        }
    }

    if (reportFile.empty()) {
        std::cerr << "testpulse: --file is required\n";
        return 2;
    }

    Config config;
    try {
        config = ResolveConfig(flags, GetEnvOrNull);
    } catch (const std::invalid_argument& e) {
        std::cerr << "testpulse: " << e.what() << "\n";
        return 2;
    }

    CurlHttpClient client;
    return RunSubmit(config, reportFile, client, std::cout, std::cerr);
}
