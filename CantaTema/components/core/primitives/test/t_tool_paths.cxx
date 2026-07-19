#include <gtest/gtest.h>
#include <primitives/tool_paths.hpp>
#include <filesystem>

namespace {

class ToolPathTest : public ::testing::Test {
protected:
    // Helper to check if a path is valid and exists
    void ExpectValidDirectory(const std::filesystem::path& path) {
        EXPECT_FALSE(path.empty()) << "Path should not be empty";
        EXPECT_TRUE(path.is_absolute()) << "Path should be absolute: " << path;
        EXPECT_TRUE(std::filesystem::exists(path)) << "Directory should exist: " << path;
        EXPECT_TRUE(std::filesystem::is_directory(path)) << "Path should be a directory: " << path;
    }
};

TEST_F(ToolPathTest, GetOSTypeReturnsExpectedValue) {
    ToolPath::OSType os = ToolPath::get_os_type();

#if defined(TOOLPATH_OS_WINDOWS)
    EXPECT_EQ(os, ToolPath::OSType::Windows);
#elif defined(TOOLPATH_OS_MACOS)
    EXPECT_EQ(os, ToolPath::OSType::MacOS);
#elif defined(TOOLPATH_OS_LINUX)
    EXPECT_EQ(os, ToolPath::OSType::Linux);
#elif defined(TOOLPATH_OS_IOS)
    EXPECT_EQ(os, ToolPath::OSType::IOS);
#elif defined(TOOLPATH_OS_ANDROID)
    EXPECT_EQ(os, ToolPath::OSType::Android);
#else
    // Fallback if no specific OS macro is matched or if running in a generic environment
    // Ensure it returns a valid enum value
    EXPECT_GE(static_cast<int>(os), static_cast<int>(ToolPath::OSType::Windows));
    EXPECT_LE(static_cast<int>(os), static_cast<int>(ToolPath::OSType::Unknown));
#endif
}

TEST_F(ToolPathTest, GetPathForFilesReturnsValidDirectory) {
    std::filesystem::path path = ToolPath::get_path_for_files();
    ExpectValidDirectory(path);
}

TEST_F(ToolPathTest, GetPathForDatabaseReturnsValidDirectory) {
    std::filesystem::path path = ToolPath::get_path_for_database();
    ExpectValidDirectory(path);
}

TEST_F(ToolPathTest, GetPathForSystemConfigReturnsValidDirectory) {
    std::filesystem::path path = ToolPath::get_path_for_system_config();
    ExpectValidDirectory(path);
}

TEST_F(ToolPathTest, GetPathForLogsReturnsValidDirectory) {
    std::filesystem::path path = ToolPath::get_path_for_logs();
    ExpectValidDirectory(path);
}

TEST_F(ToolPathTest, GetPathForSubjectReturnsValidDirectory) {
    unsigned int user_id = 1;
    unsigned int subject_id = 101;
    std::filesystem::path path = ToolPath::get_path_for_subject(user_id, subject_id);
    ExpectValidDirectory(path);
}

TEST_F(ToolPathTest, GetPathForPracticeEventReturnsValidDirectory) {
    unsigned int user_id = 1;
    unsigned int subject_id = 101;
    std::filesystem::path path = ToolPath::get_path_for_practice_event(user_id, subject_id);
    ExpectValidDirectory(path);
}

TEST_F(ToolPathTest, GetPathForModelsWhisperReturnsValidDirectory) {
    std::filesystem::path path = ToolPath::get_path_for_models_whisper();
    ExpectValidDirectory(path);
}

TEST_F(ToolPathTest, GetFilesystemPathSucceeds) {
    std::string path = ToolPath::get_filesystem_path("CantaTemaTest", "CantaTemaTest");
    EXPECT_FALSE(path.empty());
}

} // namespace

#include <primitives/utils_logger.hpp>

TEST(ToolPathGlobalTest, InitializeStandardLogger) {
    // Save current logger to restore later
    spdlog::logger* old_logger = logger;
    
    // Call normal initialization
    util_logger_init();
    
    // Verify it was initialized
    EXPECT_NE(logger, nullptr);
    
    // Restore
    if (logger != old_logger) {
        delete logger;
        logger = old_logger;
    }
}