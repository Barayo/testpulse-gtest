#pragma once

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace testpulse {

// Thrown by Attach() for every rejection case (unsupported content type,
// no active test, undeclared case key) -- never a null-pointer dereference
// or another unrelated exception type.
class TestPulseError : public std::runtime_error {
public:
    explicit TestPulseError(const std::string& message) : std::runtime_error(message) {}
};

namespace detail {

struct CaseOptions {
    std::optional<std::string> platform;
    std::optional<std::string> version;
    std::optional<std::vector<std::string>> tags;
};

struct PlatformOption {
    std::string value;
    void Apply(CaseOptions& opts) const { opts.platform = value; }
};

struct VersionOption {
    std::string value;
    void Apply(CaseOptions& opts) const { opts.version = value; }
};

struct TagsOption {
    std::vector<std::string> value;
    void Apply(CaseOptions& opts) const { opts.tags = value; }
};

inline std::string JoinTags(const std::vector<std::string>& tags) {
    std::string joined;
    for (size_t i = 0; i < tags.size(); ++i) {
        if (i > 0) {
            joined += ",";
        }
        joined += tags[i];
    }
    return joined;
}

// The internal allowlist: which case keys has the currently-running test
// (identified by "<test_suite_name>.<name>") declared via Case()? Attach()
// checks this before accepting an attachment -- necessary because
// RecordProperty() itself has no return value or registry to query back
// from. A function-local static inside an inline function is the standard
// header-only-library idiom for a single, safe instance of mutable global
// state across translation units. Mutex-guarded defensively: gtest runs
// single-threaded per binary by default (no built-in test parallelism the
// way xUnit/Gradle have), so this is not exercised by genuine concurrency
// in the default case -- see design.md's concurrency scoping note.
struct AllowlistState {
    std::mutex mutex;
    std::unordered_map<std::string, std::unordered_set<std::string>> declared;
};

inline AllowlistState& GetAllowlistState() {
    static AllowlistState state;
    return state;
}

// Empty when no test is currently running (current_test_info() is null).
inline std::string CurrentTestIdentity() {
    const ::testing::TestInfo* info = ::testing::UnitTest::GetInstance()->current_test_info();
    if (info == nullptr) {
        return {};
    }
    return std::string(info->test_suite_name()) + "." + info->name();
}

inline void RecordDeclaredCaseKey(const std::string& testIdentity, const std::string& caseKey) {
    AllowlistState& state = GetAllowlistState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.declared[testIdentity].insert(caseKey);
}

inline bool IsDeclaredCaseKey(const std::string& testIdentity, const std::string& caseKey) {
    AllowlistState& state = GetAllowlistState();
    std::lock_guard<std::mutex> lock(state.mutex);
    auto it = state.declared.find(testIdentity);
    if (it == state.declared.end()) {
        return false;
    }
    return it->second.count(caseKey) > 0;
}

inline uint64_t NextAttachmentId() {
    static std::atomic<uint64_t> counter{0};
    return counter.fetch_add(1, std::memory_order_relaxed);
}

// Invocation-unique, never derived from the caller-supplied filename --
// no path-traversal surface, matching every other plugin's attachment
// storage. Uniqueness (not cryptographic strength) is all that's needed
// here, so a plain std::hash combine is sufficient.
inline std::string HashInvocation(const std::string& caseKey, uint64_t id) {
    std::hash<std::string> hasher;
    std::ostringstream oss;
    oss << std::hex << hasher(caseKey + ":" + std::to_string(id));
    return oss.str();
}

inline std::string JsonString(const std::string& value) {
    std::string out = "\"";
    for (char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    out += "\"";
    return out;
}

// The test binary and the later `testpulse-gtest submit` CLI invocation
// are separate processes, so attachments are handed off via disk -- a
// `.testpulse/attachments` directory relative to the current working
// directory, which `submit`'s `--dir` flag (default `.`) then searches
// recursively.
inline std::filesystem::path ScratchDir() {
    return std::filesystem::current_path() / ".testpulse" / "attachments";
}

inline void WriteAttachment(const std::string& caseKey, const std::vector<uint8_t>& data,
                             const std::string& filename, const std::string& contentType) {
    std::filesystem::path dir = ScratchDir();
    std::filesystem::create_directories(dir);

    uint64_t id = NextAttachmentId();
    std::string hash = HashInvocation(caseKey, id);

    std::ofstream dataFile(dir / (hash + ".data"), std::ios::binary);
    dataFile.write(reinterpret_cast<const char*>(data.data()),
                    static_cast<std::streamsize>(data.size()));

    std::ofstream metaFile(dir / (hash + ".json"));
    metaFile << "{" << "\"caseKey\":" << JsonString(caseKey) << ","
              << "\"filename\":" << JsonString(filename) << ","
              << "\"contentType\":" << JsonString(contentType) << "}";
}

}  // namespace detail

inline detail::PlatformOption WithPlatform(std::string platform) {
    return detail::PlatformOption{std::move(platform)};
}

inline detail::VersionOption WithVersion(std::string version) {
    return detail::VersionOption{std::move(version)};
}

inline detail::TagsOption WithTags(std::vector<std::string> tags) {
    return detail::TagsOption{std::move(tags)};
}

// Tags the currently-running test with a TestPulse case key via gtest's
// own native RecordProperty() -- a static member function, so no test
// handle needs to be passed in, unlike testpulse-go's Case(t, caseKey,
// opts...). Also records the declared case key against the current
// test's identity internally, since Attach() needs some record of "which
// case keys did this test declare" to enforce its own allowlist check.
template <typename... Opts>
inline void Case(const std::string& caseKey, Opts&&... opts) {
    detail::CaseOptions options;
    (opts.Apply(options), ...);

    ::testing::Test::RecordProperty("testpulse_case_key", caseKey);
    if (options.platform) {
        ::testing::Test::RecordProperty("testpulse_platform", *options.platform);
    }
    if (options.version) {
        ::testing::Test::RecordProperty("testpulse_version", *options.version);
    }
    if (options.tags) {
        ::testing::Test::RecordProperty("testpulse_tags", detail::JoinTags(*options.tags));
    }

    std::string identity = detail::CurrentTestIdentity();
    if (!identity.empty()) {
        detail::RecordDeclaredCaseKey(identity, caseKey);
    }
}

// Records a screenshot/artifact attachment for caseKey, which must equal
// one the currently-executing test has itself declared via Case().
// Identified via gtest's current_test_info(), not stack-frame inference.
inline void Attach(const std::string& caseKey, const std::vector<uint8_t>& data,
                    const std::string& filename, const std::string& contentType) {
    static const std::unordered_set<std::string> kAllowedContentTypes = {"image/png", "image/jpeg",
                                                                           "image/webp"};
    if (kAllowedContentTypes.count(contentType) == 0) {
        throw TestPulseError("testpulse: unsupported content type '" + contentType +
                              "' (allowed: image/png, image/jpeg, image/webp)");
    }

    std::string identity = detail::CurrentTestIdentity();
    if (identity.empty()) {
        throw TestPulseError(
            "testpulse: Attach called outside an active test execution "
            "(current_test_info() was null -- e.g. called from global "
            "setup/teardown outside any TEST/TEST_F body)");
    }

    if (!detail::IsDeclaredCaseKey(identity, caseKey)) {
        throw TestPulseError("testpulse: case key '" + caseKey +
                              "' was not declared via Case() by the currently-executing test '" +
                              identity + "'");
    }

    detail::WriteAttachment(caseKey, data, filename, contentType);
}

}  // namespace testpulse
