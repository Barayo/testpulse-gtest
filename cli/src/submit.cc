#include <testpulse_cli/submit.hpp>

#include <testpulse_cli/attachments.hpp>
#include <testpulse_cli/report_reader.hpp>

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <unordered_set>

namespace testpulse_cli {

namespace {

std::string ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

// A minimal, dependency-free base64 encoder -- the only place attachment
// bytes need encoding for the JSON request body.
std::string Base64Encode(const std::vector<uint8_t>& data) {
    static const char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t i = 0;
    while (i + 2 < data.size()) {
        uint32_t chunk = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out += kTable[(chunk >> 18) & 0x3F];
        out += kTable[(chunk >> 12) & 0x3F];
        out += kTable[(chunk >> 6) & 0x3F];
        out += kTable[chunk & 0x3F];
        i += 3;
    }
    size_t remaining = data.size() - i;
    if (remaining == 1) {
        uint32_t chunk = data[i] << 16;
        out += kTable[(chunk >> 18) & 0x3F];
        out += kTable[(chunk >> 12) & 0x3F];
        out += "==";
    } else if (remaining == 2) {
        uint32_t chunk = (data[i] << 16) | (data[i + 1] << 8);
        out += kTable[(chunk >> 18) & 0x3F];
        out += kTable[(chunk >> 12) & 0x3F];
        out += kTable[(chunk >> 6) & 0x3F];
        out += "=";
    }
    return out;
}

std::string ImportsUrl(const Config& config) {
    return config.url + "/api/v1/projects/" + config.project + "/imports";
}

std::string CasesUrl(const Config& config) {
    return config.url + "/api/v1/projects/" + config.project + "/cases";
}

int RunDryRun(const Config& config, const std::string& reportXml, HttpClient& client,
              std::ostream& out, std::ostream& err) {
    out << "testpulse: dry run -- no import will be submitted\n";

    HttpResponse response = client.Get(CasesUrl(config), config.token);
    if (response.networkError || response.statusCode < 200 || response.statusCode >= 300) {
        err << "testpulse: dry-run fetch failed: "
            << (response.networkError ? response.networkErrorMessage : response.body) << "\n";
        return 1;
    }

    std::unordered_set<std::string> existing;
    try {
        nlohmann::json cases = nlohmann::json::parse(response.body);
        for (const auto& c : cases) {
            existing.insert(c.at("key").get<std::string>());
        }
    } catch (const std::exception& e) {
        err << "testpulse: dry-run fetch returned malformed JSON: " << e.what() << "\n";
        return 1;
    }

    for (const std::string& key : ExtractDeclaredCaseKeys(reportXml)) {
        if (existing.count(key) > 0) {
            out << "  would match: " << key << "\n";
        } else {
            out << "  would NOT match (no such case): " << key << "\n";
        }
    }
    return 0;
}

int RunRealSubmit(const Config& config, const std::string& reportXml, HttpClient& client,
                   std::ostream& out, std::ostream& err) {
    // ReadAttachments has no awareness of which report is being
    // submitted -- it recursively scans all of config.dir, so a stale
    // .testpulse/attachments left over from an unrelated earlier test
    // binary/run (a real, ordinary occurrence in a shared CI workspace,
    // not just a contrived scenario) would otherwise silently ride along
    // into this submission. Filtering to only the case keys THIS report
    // actually declares is what makes ReadAttachments' directory-wide
    // scan safe to use as-is.
    std::unordered_set<std::string> declaredInThisReport;
    for (const std::string& key : ExtractDeclaredCaseKeys(reportXml)) {
        declaredInThisReport.insert(key);
    }

    std::vector<StoredAttachment> attachments = ReadAttachments(config.dir);

    nlohmann::json attachmentsJson = nlohmann::json::array();
    for (const auto& a : attachments) {
        if (declaredInThisReport.count(a.caseKey) == 0) {
            continue;
        }
        attachmentsJson.push_back({{"caseKey", a.caseKey},
                                    {"filename", a.filename},
                                    {"contentType", a.contentType},
                                    {"data", Base64Encode(a.data)}});
    }

    nlohmann::json body = {
        {"format", "junit-xml"},
        {"report", reportXml},
        {"attachments", attachmentsJson},
    };

    HttpResponse response = client.Post(ImportsUrl(config), body.dump(), config.token);

    if (response.networkError) {
        err << "testpulse: submission failed: " << response.networkErrorMessage << "\n";
        return 1;
    }

    if (response.statusCode == 201) {
        try {
            nlohmann::json parsed = nlohmann::json::parse(response.body);
            out << "testpulse: all tests matched, created run " << parsed.value("key", "") << "\n";
        } catch (const std::exception&) {
            out << "testpulse: all tests matched\n";
        }
        return 0;
    }

    if (response.statusCode == 207) {
        try {
            nlohmann::json parsed = nlohmann::json::parse(response.body);
            int matched = parsed.value("matched", 0);
            auto unmatched = parsed.value("unmatched", nlohmann::json::array());
            out << "testpulse: " << matched << " matched, " << unmatched.size() << " unmatched\n";
            for (const auto& u : unmatched) {
                out << "  unmatched: " << u.value("caseKey", "") << "\n";
            }
            if (!unmatched.empty()) {
                out << "testpulse: use --fail-on-unmatched to make this a hard failure\n";
                return config.failOnUnmatched ? 1 : 0;
            }
            return 0;
        } catch (const std::exception& e) {
            err << "testpulse: 207 response was malformed JSON: " << e.what() << "\n";
            return 1;
        }
    }

    err << "testpulse: submission failed: status " << response.statusCode << ": " << response.body
        << "\n";
    return 1;
}

}  // namespace

int RunSubmit(const Config& config, const std::string& reportPath, HttpClient& client,
              std::ostream& out, std::ostream& err) {
    std::string reportXml = ReadFile(reportPath);

    if (config.dryRun) {
        return RunDryRun(config, reportXml, client, out, err);
    }
    return RunRealSubmit(config, reportXml, client, out, err);
}

}  // namespace testpulse_cli
