#pragma once

#include <string>
#include <vector>

namespace testpulse_cli {

// Scans a JUnit XML report (as produced by --gtest_output=xml, with
// testpulse::Case()-injected properties) for every
// <property name="testpulse_case_key" value="..."/> and returns the
// declared case keys, in document order, duplicates included. This is
// intentionally NOT a general-purpose XML parser -- the input is always
// self-produced by gtest's own RecordProperty() writer, a long-stable,
// narrow, well-known shape, the same "target the long-stable subset"
// reasoning every other plugin in this family already applies.
std::vector<std::string> ExtractDeclaredCaseKeys(const std::string& reportXml);

}  // namespace testpulse_cli
