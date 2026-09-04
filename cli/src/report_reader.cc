#include <testpulse_cli/report_reader.hpp>

namespace testpulse_cli {

namespace {

// Finds the value attribute of the next <property name="wantedName"
// value="..."/> at or after `from`, returning std::string::npos via an
// empty optional-style pair if none remains.
bool FindNextProperty(const std::string& xml, const std::string& wantedName, size_t from,
                       std::string& outValue, size_t& outEnd) {
    size_t pos = from;
    while (true) {
        size_t tagStart = xml.find("<property", pos);
        if (tagStart == std::string::npos) {
            return false;
        }
        size_t tagEnd = xml.find("/>", tagStart);
        if (tagEnd == std::string::npos) {
            tagEnd = xml.find('>', tagStart);
        }
        if (tagEnd == std::string::npos) {
            return false;
        }
        std::string tag = xml.substr(tagStart, tagEnd - tagStart);

        std::string namePattern = "name=\"" + wantedName + "\"";
        if (tag.find(namePattern) != std::string::npos) {
            size_t valuePos = tag.find("value=\"");
            if (valuePos != std::string::npos) {
                valuePos += 7;  // strlen("value=\"")
                size_t valueEnd = tag.find('"', valuePos);
                if (valueEnd != std::string::npos) {
                    outValue = tag.substr(valuePos, valueEnd - valuePos);
                    outEnd = tagEnd;
                    return true;
                }
            }
        }
        pos = tagEnd;
    }
}

}  // namespace

std::vector<std::string> ExtractDeclaredCaseKeys(const std::string& reportXml) {
    std::vector<std::string> keys;
    size_t pos = 0;
    std::string value;
    size_t end;
    while (FindNextProperty(reportXml, "testpulse_case_key", pos, value, end)) {
        keys.push_back(value);
        pos = end;
    }
    return keys;
}

}  // namespace testpulse_cli
