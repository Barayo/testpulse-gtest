#include <testpulse_cli/attachments.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace testpulse_cli {

std::vector<StoredAttachment> ReadAttachments(const std::string& searchRoot) {
    std::vector<StoredAttachment> result;
    if (!fs::exists(searchRoot)) {
        return result;
    }

    for (const auto& entry : fs::recursive_directory_iterator(searchRoot)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        if (entry.path().parent_path().filename() != "attachments") {
            continue;
        }

        std::ifstream metaFile(entry.path());
        std::ostringstream metaStream;
        metaStream << metaFile.rdbuf();
        nlohmann::json meta = nlohmann::json::parse(metaStream.str());

        fs::path dataPath = entry.path();
        dataPath.replace_extension(".data");
        if (!fs::exists(dataPath)) {
            continue;
        }
        std::ifstream dataFile(dataPath, std::ios::binary);
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(dataFile)),
                                   std::istreambuf_iterator<char>());

        StoredAttachment attachment;
        attachment.caseKey = meta.at("caseKey").get<std::string>();
        attachment.filename = meta.at("filename").get<std::string>();
        attachment.contentType = meta.at("contentType").get<std::string>();
        attachment.data = std::move(data);
        result.push_back(std::move(attachment));
    }

    return result;
}

}  // namespace testpulse_cli
