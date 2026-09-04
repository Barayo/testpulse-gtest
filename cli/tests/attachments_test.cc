#include <gtest/gtest.h>
#include <testpulse_cli/attachments.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using testpulse_cli::ReadAttachments;

namespace {

void WriteSidecar(const fs::path& dir, const std::string& hash, const std::string& caseKey,
                   const std::string& filename, const std::string& contentType,
                   const std::vector<uint8_t>& data) {
    fs::create_directories(dir);
    std::ofstream dataFile(dir / (hash + ".data"), std::ios::binary);
    dataFile.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

    std::ofstream metaFile(dir / (hash + ".json"));
    metaFile << R"({"caseKey":")" << caseKey << R"(","filename":")" << filename
              << R"(","contentType":")" << contentType << R"("})";
}

}  // namespace

TEST(AttachmentsTest, ReadsASingleAttachment) {
    fs::path root = fs::temp_directory_path() / "testpulse_cli_attach_single";
    fs::remove_all(root);
    WriteSidecar(root / ".testpulse" / "attachments", "abc123", "LOGIN-42", "failure.png",
                 "image/png", {0x01, 0x02, 0x03});

    auto attachments = ReadAttachments(root.string());
    ASSERT_EQ(attachments.size(), 1u);
    EXPECT_EQ(attachments[0].caseKey, "LOGIN-42");
    EXPECT_EQ(attachments[0].filename, "failure.png");
    EXPECT_EQ(attachments[0].contentType, "image/png");
    EXPECT_EQ(attachments[0].data, (std::vector<uint8_t>{0x01, 0x02, 0x03}));
}

TEST(AttachmentsTest, ReadsMultipleAttachmentsFromNestedDirectories) {
    fs::path root = fs::temp_directory_path() / "testpulse_cli_attach_nested";
    fs::remove_all(root);
    WriteSidecar(root / "pkg1" / ".testpulse" / "attachments", "hash1", "LOGIN-1", "a.png",
                 "image/png", {0x01});
    WriteSidecar(root / "pkg2" / ".testpulse" / "attachments", "hash2", "LOGIN-2", "b.png",
                 "image/png", {0x02});

    auto attachments = ReadAttachments(root.string());
    EXPECT_EQ(attachments.size(), 2u);
}

TEST(AttachmentsTest, NoAttachmentsReturnsEmpty) {
    fs::path root = fs::temp_directory_path() / "testpulse_cli_attach_none";
    fs::remove_all(root);
    fs::create_directories(root);
    EXPECT_TRUE(ReadAttachments(root.string()).empty());
}
