#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace testpulse_cli {

struct StoredAttachment {
    std::string caseKey;
    std::string filename;
    std::string contentType;
    std::vector<uint8_t> data;
};

// Recursively searches searchRoot for .testpulse/attachments directories
// (a build with several test binaries can have more than one, each in its
// own working directory) and reads every <hash>.json + <hash>.data pair
// found, matching testpulse::detail::WriteAttachment's on-disk shape.
std::vector<StoredAttachment> ReadAttachments(const std::string& searchRoot);

}  // namespace testpulse_cli
